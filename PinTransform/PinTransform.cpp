#include "PinTransform.h"
#include "AEFX_SuiteHelper.h"
#include "Smart_Utils.h"

#include "AEOGLInterop.hpp"
#include "AEUtils.hpp"

#include "../Debug.h"
#include "ComputeMat3.hpp"
#include "Settings.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/string_cast.hpp>

#include <sstream>

namespace {
std::string VertShaderPath;
std::string FragShaderPath;
} // namespace

static PF_Err About(PF_InData *in_data, PF_OutData *out_data,
                    PF_ParamDef *params[], PF_LayerDef *output) {
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    suites.ANSICallbacksSuite1()->sprintf(
        out_data->return_msg, "%s v%d.%d\r%s", FX_SETTINGS_NAME, MAJOR_VERSION,
        MINOR_VERSION, FX_SETTINGS_DESCRIPTION);
    return PF_Err_NONE;
}

static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output) {
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION, BUG_VERSION,
                                      STAGE_VERSION, BUILD_VERSION);

    // Enable 16bpc
    out_data->out_flags =
        PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_SEND_UPDATE_PARAMS_UI;

    // Enable 32bpc and SmartFX
    out_data->out_flags2 =
        PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_SUPPORTS_SMART_RENDER;

    // Initialize globalData
    auto handleSuite = suites.HandleSuite1();
    PF_Handle globalDataH = handleSuite->host_new_handle(sizeof(GlobalData));

    if (!globalDataH) {
        return PF_Err_OUT_OF_MEMORY;
    }

    out_data->global_data = globalDataH;

    GlobalData *globalData = reinterpret_cast<GlobalData *>(
        handleSuite->host_lock_handle(globalDataH));

    // Initialize global OpenGL context
    if (!OGL::globalSetup(&globalData->globalContext)) {
        err = PF_Err_OUT_OF_MEMORY;
    }

    OGL::initTexture(&globalData->inputTexture);

    handleSuite->host_unlock_handle(globalDataH);

    // Retrieve shader path
    std::string resourcePath = AEUtils::getResourcesPath(in_data);
    VertShaderPath = resourcePath + "shaders/shader.vert";
    FragShaderPath = resourcePath + "shaders/shader.frag";

    return err;
}

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output) {
    PF_Err err = PF_Err_NONE;

    // Define parameters
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);
    def.flags |= PF_ParamFlag_SUPERVISE;
    PF_ADD_POPUP("Pin Type", 4, 1,
                 "1 Pin (Translate)|"
                 "2 Pins (Trans/Scale/Rot)|"
                 "3 Pins (Pos/Scale/Rot/Skew)|"
                 "4 Pins (Perspective)",
                 PARAM_PINTYPE);

    for (int i = 0; i < 4; i++) {
        std::ostringstream label;
        label << "Source Point " << (i + 1);
        AEFX_CLR_STRUCT(def);
        def.flags |= PF_ParamFlag_SUPERVISE;
        PF_ADD_POINT(label.str().c_str(), 0, 0, false, PARAM_SRC_1 + i);
    }

    for (int i = 0; i < 4; i++) {
        std::ostringstream label;
        label << "Destination Point " << (i + 1);
        AEFX_CLR_STRUCT(def);
        def.flags |= PF_ParamFlag_SUPERVISE;
        PF_ADD_POINT(label.str().c_str(), 0, 0, false, PARAM_DST_1 + i);
    }

    out_data->num_params = PARAM_NUM_PARAMS;

    return err;
}

static PF_Err GlobalSetdown(PF_InData *in_data, PF_OutData *out_data,
                            PF_ParamDef *params[], PF_LayerDef *output) {
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    // Dispose globalData
    auto *globalData = reinterpret_cast<GlobalData *>(
        suites.HandleSuite1()->host_lock_handle(in_data->global_data));

    OGL::globalSetdown(&globalData->globalContext);
    OGL::disposeTexture(&globalData->inputTexture);

    suites.HandleSuite1()->host_dispose_handle(in_data->global_data);

    // Dispose static variables
    VertShaderPath.clear();
    FragShaderPath.clear();

    return err;
}

static PF_Err PreRender(PF_InData *in_data, PF_OutData *out_data,
                        PF_PreRenderExtra *extra) {
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    auto handleSuite = suites.HandleSuite1();

    PF_RenderRequest req = extra->input->output_request;
    PF_CheckoutResult in_result;

    // Create paramInfo
    PF_Handle paramInfoH = handleSuite->host_new_handle(sizeof(ParamInfo));

    if (!paramInfoH) {
        return PF_Err_OUT_OF_MEMORY;
    }

    // Set handler
    extra->output->pre_render_data = paramInfoH;

    ParamInfo *paramInfo = reinterpret_cast<ParamInfo *>(
        handleSuite->host_lock_handle(paramInfoH));

    if (!paramInfo) {
        err = PF_Err_OUT_OF_MEMORY;
    }

    // Assign latest param values
    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_PINTYPE, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));

    A_long pinType = param_def.u.pd.value;

    A_FloatPoint src1, src2, src3, src4;
    A_FloatPoint dst1, dst2, dst3, dst4;

    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_SRC_1,
                                    AEOGLInterop::AE_SPACE, &src1));
    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_SRC_2,
                                    AEOGLInterop::AE_SPACE, &src2));
    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_SRC_3,
                                    AEOGLInterop::AE_SPACE, &src3));
    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_SRC_4,
                                    AEOGLInterop::AE_SPACE, &src4));

    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_DST_1,
                                    AEOGLInterop::AE_SPACE, &dst1));
    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_DST_2,
                                    AEOGLInterop::AE_SPACE, &dst2));
    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_DST_3,
                                    AEOGLInterop::AE_SPACE, &dst3));
    ERR(AEOGLInterop::getPointParam(in_data, out_data, PARAM_DST_4,
                                    AEOGLInterop::AE_SPACE, &dst4));

    glm::mat3 xform = glm::mat3(1);

    switch (pinType) {
    case 1: { // Translate
        glm::vec2 trans(dst1.x - src1.x, dst1.y - src1.y);
        xform = glm::translate(xform, trans);
        FX_LOG(glm::to_string(trans));
        FX_LOG(glm::to_string(xform));
        break;
    }
    case 2: { // PSR
        glm::vec2 srcOrigin = glm::vec2(src1.x, src1.y);
        glm::vec2 srcAxisX = glm::vec2(src2.x, src2.y) - srcOrigin;
        
        glm::mat3 srcRot = glm::mat3(1);
        srcRot[0][0] = srcAxisX.x;
        srcRot[1][0] = srcAxisX.y;
        srcRot[0][1] = srcAxisX.y;
        srcRot[1][1] = -srcAxisX.x;
        
        glm::mat3 srcTrans = glm::translate(glm::mat3(1), srcOrigin);
        glm::mat3 srcXform = srcTrans * srcRot;
        
        FX_LOG("srcXform=" << glm::to_string(srcXform));

        glm::vec2 dstOrigin = glm::vec2(dst1.x, dst1.y);
        glm::vec2 dstAxisX = glm::vec2(dst2.x, dst2.y) - dstOrigin;
        
        glm::mat3 dstRot = glm::mat3(1);
        dstRot[0][0] = dstAxisX.x;
        dstRot[1][0] = dstAxisX.y;
        dstRot[0][1] = dstAxisX.y;
        dstRot[1][1] = -dstAxisX.x;
        
        glm::mat3 dstTrans = glm::translate(glm::mat3(1), dstOrigin);
        glm::mat3 dstXform = dstTrans * dstRot;
        
        FX_LOG("dstTrans=" << glm::to_string(dstTrans));
        FX_LOG("dstRot=" << glm::to_string(dstRot));
        FX_LOG("dstXform=" << glm::to_string(dstXform));
        
        
        FX_LOG("srcXformInv=" << glm::to_string(glm::inverse(dstXform)));
        

        xform = dstXform * glm::inverse(srcXform);
        
        FX_LOG("xform=" << glm::to_string(xform));
        
        break;
    }
    case 3: { // Affine transform
        glm::vec2 srcOrigin = glm::vec2(src1.x, src1.y);
        glm::vec2 srcAxisX = glm::vec2(src2.x, src2.y) - srcOrigin;
        glm::vec2 srcAxisY = glm::vec2(src3.x, src3.y) - srcOrigin;
        glm::mat3 srcXform = glm::translate(glm::mat3(1), srcOrigin);
        srcXform[0][0] = srcAxisX.x;
        srcXform[0][1] = srcAxisX.y;
        srcXform[1][0] = srcAxisY.x;
        srcXform[1][1] = srcAxisY.y;

        glm::vec2 dstOrigin = glm::vec2(dst1.x, dst1.y);
        glm::vec2 dstAxisX = glm::vec2(dst2.x, dst2.y) - dstOrigin;
        glm::vec2 dstAxisY = glm::vec2(dst3.x, dst3.y) - dstOrigin;
        glm::mat3 dstXform = glm::translate(glm::mat3(1), dstOrigin);
        dstXform[0][0] = dstAxisX.x;
        dstXform[0][1] = dstAxisX.y;
        dstXform[1][0] = dstAxisY.x;
        dstXform[1][1] = dstAxisY.y;

        xform = glm::inverse(srcXform) * dstXform;
        break;
    }
    case 4: { // Homogeneous transformations
        glm::vec2 src[4] = {
            glm::vec2(src1.x, src1.y), glm::vec2(src2.x, src2.y),
            glm::vec2(src3.x, src3.y), glm::vec2(src4.x, src4.y)};
        glm::vec2 dst[4] = {
            glm::vec2(dst1.x, dst1.y), glm::vec2(dst2.x, dst2.y),
            glm::vec2(dst3.x, dst3.y), glm::vec2(dst4.x, dst4.y)};

        xform = ComputeMat3::computePerspective(src, dst);
    }
    }

    std::memcpy(&paramInfo->xform, &xform, sizeof(glm::mat3));

    handleSuite->host_unlock_handle(paramInfoH);

    // Checkout input image
    ERR(extra->cb->checkout_layer(in_data->effect_ref, PARAM_INPUT, PARAM_INPUT,
                                  &req, in_data->current_time,
                                  in_data->time_step, in_data->time_scale,
                                  &in_result));

    // Compute the rect to render
    if (!err) {
        UnionLRect(&in_result.result_rect, &extra->output->result_rect);
        UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);
    }

    return err;
}

static PF_Err SmartRender(PF_InData *in_data, PF_OutData *out_data,
                          PF_SmartRenderExtra *extra) {
    FX_LOG("=====SmartRender=====");
    FX_LOG_TIME_START(smartRenderTime);

    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    AEGP_SuiteHandler suites(in_data->pica_basicP);

    PF_EffectWorld *input_worldP = nullptr, *output_worldP = nullptr;
    PF_WorldSuite2 *wsP = nullptr;

    // Retrieve paramInfo
    auto handleSuite = suites.HandleSuite1();

    ParamInfo *paramInfo =
        reinterpret_cast<ParamInfo *>(handleSuite->host_lock_handle(
            reinterpret_cast<PF_Handle>(extra->input->pre_render_data)));

    // Checkout layer pixels
    ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, PARAM_INPUT,
                                          &input_worldP)));
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

    // Setup wsP
    ERR(AEFX_AcquireSuite(in_data, out_data, kPFWorldSuite,
                          kPFWorldSuiteVersion2, "Couldn't load suite.",
                          (void **)&wsP));

    // Get pixel format
    PF_PixelFormat format = PF_PixelFormat_INVALID;
    ERR(wsP->PF_GetPixelFormat(input_worldP, &format));

    auto *globalData = reinterpret_cast<GlobalData *>(
        handleSuite->host_lock_handle(in_data->global_data));

    // OpenGL
    if (!err) {
        OGL::makeGlobalContextCurrent(&globalData->globalContext);

        auto ctx = OGL::getCurrentThreadRenderContext();

        GLenum glFormat = 0;
        size_t pixelSize = 0;

        switch (format) {
        case PF_PixelFormat_ARGB32:
            glFormat = GL_UNSIGNED_BYTE;
            pixelSize = sizeof(PF_Pixel8);
            break;
        case PF_PixelFormat_ARGB64:
            glFormat = GL_UNSIGNED_SHORT;
            pixelSize = sizeof(PF_Pixel16);
            break;
        case PF_PixelFormat_ARGB128:
            glFormat = GL_FLOAT;
            pixelSize = sizeof(PF_PixelFloat);
            break;
        }

        // Setup render context
        OGL::setupRenderContext(ctx, input_worldP->width, input_worldP->height,
                                glFormat, VertShaderPath, FragShaderPath);

        FX_LOG("Size=(" << ctx->width << ", " << ctx->height << ")");

        // Allocate pixels buffer
        PF_Handle pixelsBufferH =
            handleSuite->host_new_handle(ctx->width * ctx->height * pixelSize);
        void *pixelsBufferP = reinterpret_cast<char *>(
            handleSuite->host_lock_handle(pixelsBufferH));

        // Upload buffer to OpenGL texture
        AEOGLInterop::uploadTexture(ctx, &globalData->inputTexture,
                                    input_worldP, pixelsBufferP);

        // Set uniforms
        OGL::setUniformTexture(ctx, "tex0", &globalData->inputTexture, 0);

        float multiplier16bit =
            ctx->format == GL_UNSIGNED_SHORT ? (65535.0f / 32768.0f) : 1.0f;
        OGL::setUniform1f(ctx, "multiplier16bit", multiplier16bit);
        
        float actualWidth = (float)in_data->width;
        float actualHeight = (float)in_data->height;
        
        FX_LOG("Actual size=(" << actualWidth << ", " << actualHeight << ")");
        FX_LOG("Matrix=" << glm::to_string(paramInfo->xform));
        
        OGL::setUniform2f(ctx, "resolution", actualWidth, actualHeight);
        
        OGL::setUniformMatrix3f(ctx, "xform", &paramInfo->xform);

        FX_LOG_TIME_START(glRenderTime);
        OGL::renderToBuffer(ctx, pixelsBufferP);
        FX_LOG_TIME_END(glRenderTime, "GL rendering");

        // downloadTexture

        FX_LOG_TIME_START(downloadTextureTime);
        ERR(AEOGLInterop::downloadTexture(ctx, pixelsBufferP, output_worldP));
        FX_LOG_TIME_END(downloadTextureTime, "Download texture");

        handleSuite->host_unlock_handle(pixelsBufferH);
        handleSuite->host_dispose_handle(pixelsBufferH);
    }

    // Check in
    ERR2(AEFX_ReleaseSuite(in_data, out_data, kPFWorldSuite,
                           kPFWorldSuiteVersion2, "Couldn't release suite."));
    ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, PARAM_INPUT));

    FX_LOG_TIME_END(smartRenderTime, "SmartRender");

    return err;
}

static PF_Err UpdateParameterUI(PF_InData *in_data, PF_OutData *out_data,
                                PF_ParamDef *params[], PF_LayerDef *outputP) {
    PF_Err err = PF_Err_NONE;

    return err;
}

extern "C" DllExport PF_Err PluginDataEntryFunction(
    PF_PluginDataPtr inPtr, PF_PluginDataCB inPluginDataCallBackPtr,
    SPBasicSuite *inSPBasicSuitePtr, const char *inHostName,
    const char *inHostVersion) {
    PF_Err result = PF_Err_INVALID_CALLBACK;

    result =
        PF_REGISTER_EFFECT(inPtr, inPluginDataCallBackPtr, FX_SETTINGS_NAME,
                           FX_SETTINGS_MATCH_NAME, FX_SETTINGS_CATEGORY,
                           AE_RESERVED_INFO); // Reserved Info

    return result;
}

PF_Err EffectMain(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data,
                  PF_ParamDef *params[], PF_LayerDef *output, void *extra) {
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
        case PF_Cmd_ABOUT:
            err = About(in_data, out_data, params, output);
            break;

        case PF_Cmd_GLOBAL_SETUP:
            err = GlobalSetup(in_data, out_data, params, output);
            break;

        case PF_Cmd_PARAMS_SETUP:
            err = ParamsSetup(in_data, out_data, params, output);
            break;

        case PF_Cmd_GLOBAL_SETDOWN:
            err = GlobalSetdown(in_data, out_data, params, output);
            break;

        case PF_Cmd_SMART_PRE_RENDER:
            err = PreRender(in_data, out_data,
                            reinterpret_cast<PF_PreRenderExtra *>(extra));
            break;

        case PF_Cmd_SMART_RENDER:
            err = SmartRender(in_data, out_data,
                              reinterpret_cast<PF_SmartRenderExtra *>(extra));
            break;
        case PF_Cmd_UPDATE_PARAMS_UI:
            err = UpdateParameterUI(in_data, out_data, params, output);
            break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}
