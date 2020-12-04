#include "PinTransform.h"

#include "AEOGLInterop.hpp"
#include "AEUtils.hpp"

#include "../Debug.h"
#include "Settings.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/string_cast.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

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
    PF_ADD_POPUP("Vieweing Mode", 2, 2, "Original|Result", PARAM_VIEWING_MODE);

    AEFX_CLR_STRUCT(def);
    def.flags |= PF_ParamFlag_SUPERVISE;
    PF_ADD_POPUP("Pin Type", 4, 4,
                 "1 Pin (Translate)|"
                 "2 Pins (Trans/Scale/Rot)|"
                 "3 Pins (Pos/Scale/Rot/Skew)|"
                 "4 Pins (Perspective)",
                 PARAM_PINCOUNT);

    int pointDefaults[4][2]{{0, 0}, {100, 0}, {0, 100}, {100, 100}};

    for (int i = 0; i < 4; i++) {
        std::ostringstream label;
        label << "Source Point " << (i + 1);
        AEFX_CLR_STRUCT(def);
        int x = pointDefaults[i][0];
        int y = pointDefaults[i][1];
        PF_ADD_POINT(label.str().c_str(), x, y, false, PARAM_SRC_1 + i);
    }

    for (int i = 0; i < 4; i++) {
        std::ostringstream label;
        label << "Destination Point " << (i + 1);
        AEFX_CLR_STRUCT(def);
        int x = pointDefaults[i][0];
        int y = pointDefaults[i][1];
        PF_ADD_POINT(label.str().c_str(), x, y, false, PARAM_DST_1 + i);
    }

    AEFX_CLR_STRUCT(def);
    PF_ADD_BUTTON("", "Copy Src->Dst", 0, PF_ParamFlag_SUPERVISE,
                  PARAM_COPY_SRC_TO_DST);

    AEFX_CLR_STRUCT(def);
    PF_ADD_BUTTON("", "Swap Src/Dst", 0, PF_ParamFlag_SUPERVISE,
                  PARAM_SWAP_SRC_DST);

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

    A_long viewingMode;
    ERR(AEOGLInterop::getPopupParam(in_data, out_data, PARAM_VIEWING_MODE,
                                    &viewingMode));
    A_long pinCount;

    if (viewingMode == PARAM_VIEWING_MODE_ORIGINAL) {
        pinCount = 0;
    } else {
        ERR(AEOGLInterop::getPopupParam(in_data, out_data, PARAM_PINCOUNT,
                                        &pinCount));
    }

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

    switch (pinCount) {
    case 0: // Original
        break;
    case 1: { // Translate
        glm::vec2 trans(dst1.x - src1.x, dst1.y - src1.y);
        xform = glm::translate(xform, trans);
        break;
    }
    case 2:
    case 3: { // Affine transformation
        std::vector<cv::Point2f> srcPoints(3);
        srcPoints[0] = cv::Point2f(src1.x, src1.y);
        srcPoints[1] = cv::Point2f(src2.x, src2.y);
        srcPoints[2] = cv::Point2f(src3.x, src3.y);

        std::vector<cv::Point2f> dstPoints(3);
        dstPoints[0] = cv::Point2f(dst1.x, dst1.y);
        dstPoints[1] = cv::Point2f(dst2.x, dst2.y);
        dstPoints[2] = cv::Point2f(dst3.x, dst3.y);

        cv::Mat mat = cv::getAffineTransform(srcPoints, dstPoints);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 2; j++) {
                xform[i][j] = (float)mat.at<double>(j, i);
            }
        }
        break;
    }
    case 4: { // Homogeneous transformation
        std::vector<cv::Point2f> srcPoints(4);
        srcPoints[0] = cv::Point2f(src1.x, src1.y);
        srcPoints[1] = cv::Point2f(src2.x, src2.y);
        srcPoints[2] = cv::Point2f(src4.x, src4.y);
        srcPoints[3] = cv::Point2f(src3.x, src3.y);

        std::vector<cv::Point2f> dstPoints(4);
        dstPoints[0] = cv::Point2f(dst1.x, dst1.y);
        dstPoints[1] = cv::Point2f(dst2.x, dst2.y);
        dstPoints[2] = cv::Point2f(dst4.x, dst4.y);
        dstPoints[3] = cv::Point2f(dst3.x, dst3.y);

        cv::Mat mat = cv::getPerspectiveTransform(srcPoints, dstPoints);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                xform[i][j] = (float)mat.at<double>(j, i);
            }
        }
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

        OGL::setUniform2f(ctx, "resolution", actualWidth, actualHeight);

        FX_LOG(glm::to_string(paramInfo->xform));

        glm::mat3 xformInv = glm::inverse(paramInfo->xform);
        OGL::setUniformMatrix3f(ctx, "xformInv", &xformInv);

        OGL::renderToBuffer(ctx, pixelsBufferP);

        // downloadTextur
        ERR(AEOGLInterop::downloadTexture(ctx, pixelsBufferP, output_worldP));

        handleSuite->host_unlock_handle(pixelsBufferH);
        handleSuite->host_dispose_handle(pixelsBufferH);
    }

    // Check in
    ERR2(AEFX_ReleaseSuite(in_data, out_data, kPFWorldSuite,
                           kPFWorldSuiteVersion2, "Couldn't release suite."));
    ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, PARAM_INPUT));

    return err;
}

static PF_Err UserChangedParam(PF_InData *in_data, PF_OutData *out_data,
                               PF_ParamDef *params[],
                               const PF_UserChangedParamExtra *which_hitP) {
    PF_Err err = PF_Err_NONE;

    switch (which_hitP->param_index) {
    case PARAM_PINCOUNT: {
        AEGP_SuiteHandler suites(in_data->pica_basicP);
        auto paramSuite = suites.ParamUtilsSuite3();

        A_long pinCount;
        ERR(AEOGLInterop::getPopupParam(in_data, out_data, PARAM_PINCOUNT,
                                        &pinCount));

        // Copy All ParamDef
        PF_ParamDef paramsCopy[PARAM_NUM_PARAMS];

        for (size_t i = 0; i < PARAM_NUM_PARAMS; i++) {
            AEFX_CLR_STRUCT(paramsCopy[i])
            paramsCopy[i] = *params[i];
        }

        for (size_t i = 0; i < 4; i++) {
            if (i + 1 <= pinCount) {
                paramsCopy[PARAM_SRC_1 + i].ui_flags &= (~PF_PUI_DISABLED);
                paramsCopy[PARAM_DST_1 + i].ui_flags &= (~PF_PUI_DISABLED);
            } else {
                paramsCopy[PARAM_SRC_1 + i].ui_flags |= PF_PUI_DISABLED;
                paramsCopy[PARAM_DST_1 + i].ui_flags |= PF_PUI_DISABLED;
            }

            ERR(paramSuite->PF_UpdateParamUI(in_data->effect_ref,
                                             PARAM_SRC_1 + i,
                                             &paramsCopy[PARAM_SRC_1 + i]));
            ERR(paramSuite->PF_UpdateParamUI(in_data->effect_ref,
                                             PARAM_DST_1 + i,
                                             &paramsCopy[PARAM_DST_1 + i]));
        }
    }
    case PARAM_COPY_SRC_TO_DST: {
        PF_ParamDef *paramSrc, *paramDst;
        for (size_t i = 0; i < 4; i++) {
            paramSrc = params[PARAM_SRC_1 + i];
            paramDst = params[PARAM_DST_1 + i];
            paramDst->u.td.x_value = paramSrc->u.td.x_value;
            paramDst->u.td.y_value = paramSrc->u.td.y_value;
            paramDst->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
            ;
        }
    }
    case PARAM_SWAP_SRC_DST: {
        PF_ParamDef *paramSrc, *paramDst;
        for (size_t i = 0; i < 4; i++) {
            paramSrc = params[PARAM_SRC_1 + i];
            paramDst = params[PARAM_DST_1 + i];

            A_long swap = paramDst->u.td.x_value;
            paramDst->u.td.x_value = paramSrc->u.td.x_value;
            paramSrc->u.td.x_value = swap;

            swap = paramDst->u.td.y_value;
            paramDst->u.td.y_value = paramSrc->u.td.y_value;
            paramSrc->u.td.y_value = swap;

            paramSrc->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
            paramDst->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
            ;
        }
    }
    }

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
        case PF_Cmd_USER_CHANGED_PARAM:
            err = UserChangedParam(
                in_data, out_data, params,
                reinterpret_cast<const PF_UserChangedParamExtra *>(extra));
            break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}
