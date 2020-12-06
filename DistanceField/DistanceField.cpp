#include "DistanceField.h"
#include "AEFX_SuiteHelper.h"
#include "Smart_Utils.h"

#include "AEOGLInterop.hpp"
#include "AEUtils.hpp"
#include "Settings.h"

#include "../Debug.h"
#include "Settings.h"

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
        PF_OutFlag_DEEP_COLOR_AWARE;

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
    globalData->globalContext = *new OGL::GlobalContext();
    if (!globalData->globalContext.initialized) {
        err = PF_Err_OUT_OF_MEMORY;
    }

    globalData->globalContext.bind();

    // Setup GL objects
    globalData->inputTexture = *new OGL::Texture();
    globalData->outputFbo = *new OGL::Fbo();
    globalData->fboA = *new OGL::Fbo();
    globalData->fboB = *new OGL::Fbo();
    globalData->quad = *new OGL::QuadVao();

    std::string shaderDir = AEUtils::getResourcesPath(in_data) + "shaders/";
    std::string vertPath = shaderDir + "passthru.vert";

    globalData->thresholdShader = *new OGL::Shader(vertPath.c_str(),
                                                   (shaderDir + "threshold.frag").c_str());
    globalData->distanceShader = *new OGL::Shader(vertPath.c_str(),
                                                  (shaderDir + "distance.frag").c_str());
    globalData->outputShader = *new OGL::Shader(vertPath.c_str(),
                                                (shaderDir + "output.frag").c_str());

    handleSuite->host_unlock_handle(globalDataH);
    return err;
}

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output) {
    PF_Err err = PF_Err_NONE;

    // Define parameters
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDER("Width",       // NAME
                        0,             // VALID_MIN,
                        2000,          // VALID_MAX
                        0,             // SLIDER_MIN
                        2000,          // SLIDER_MAX
                        200,           // CURVE_TORELANCE
                        100,           // DFLT
                        0,             // PREC
                        0,             // DISP
                        0,             // WANT_PHASE
                        PARAM_WIDTH);  // ID

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

    // Explicitly call deconstructor
    globalData->inputTexture.~Texture();
    globalData->thresholdShader.~Shader();
    globalData->distanceShader.~Shader();
    globalData->outputShader.~Shader();
    globalData->outputFbo.~Fbo();
    globalData->fboA.~Fbo();
    globalData->fboB.~Fbo();
    globalData->quad.~QuadVao();
    globalData->globalContext.~GlobalContext();

    suites.HandleSuite1()->host_dispose_handle(in_data->global_data);

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
    ERR(AEOGLInterop::getFloatSliderParam(in_data, out_data, PARAM_WIDTH,
                                          &paramInfo->width));

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
        globalData->globalContext.bind();

        GLenum pixelType;
        switch (format) {
            case PF_PixelFormat_ARGB32:
                pixelType = GL_UNSIGNED_BYTE;
                break;
            case PF_PixelFormat_ARGB64:
                pixelType = GL_UNSIGNED_SHORT;
                break;
            case PF_PixelFormat_ARGB128:
                pixelType = GL_FLOAT;
                break;
        }

        GLsizei width = input_worldP->width;
        GLsizei height = input_worldP->height;
        size_t pixelBytes = AEOGLInterop::getPixelBytes(pixelType);

        GLfloat infinityValue = 30000.0f;
        //glGetMinmax(GL_MINMAX, GL_TRUE, GL_RGBA, GL_FLOAT, &maxValue);

        // Setup render context
        globalData->fboA.allocate(width, height, GL_FLOAT);
        globalData->fboB.allocate(width, height, GL_FLOAT);
        globalData->outputFbo.allocate(width, height, pixelType);
        globalData->inputTexture.allocate(width, height, pixelType);

        // Allocate pixels buffer
        PF_Handle pixelsBufferH =
            handleSuite->host_new_handle(width * height * pixelBytes);
        void *pixelsBufferP = reinterpret_cast<char *>(
            handleSuite->host_lock_handle(pixelsBufferH));

        // Upload buffer to OpenGL texture
        AEOGLInterop::uploadTexture(&globalData->inputTexture,
                                    input_worldP, pixelsBufferP, pixelType);

        float multiplier16bit = AEOGLInterop::getMultiplier16bit(pixelType);
        
        float downsampleX = (float)in_data->downsample_x.num / in_data->downsample_x.den;
        // float downsampleY = (float)in_data->downsample_y.num / in_data->downsample_y.den;
        
        int distanceWidth = paramInfo->width * downsampleX;

        // input -> float
        globalData->fboA.bind();
        globalData->thresholdShader.bind();
        globalData->thresholdShader.setTexture("tex0", &globalData->inputTexture, 0);
        globalData->thresholdShader.setFloat("multiplier16bit", multiplier16bit);
        globalData->thresholdShader.setFloat("infinity", infinityValue);
        globalData->quad.render();

        // Compute distance
        globalData->distanceShader.bind();

        OGL::Fbo *fboSrc = &globalData->fboA;
        OGL::Fbo *fboDst = &globalData->fboB;
        
        // Horizontal
        for (int i = 0; i < distanceWidth; i++) {
            float beta = 2 * i + 1;

            fboDst->bind();
            globalData->distanceShader.setTexture("tex0", fboSrc->getTexture(), 0);
            globalData->distanceShader.setFloat("beta", beta);
            globalData->distanceShader.setVec2("offset", 1.0f / (float)width, 0.0f);
            globalData->quad.render();

            OGL::Fbo *swap = fboDst;
            fboDst = fboSrc;
            fboSrc = swap;
        }
        
        // Vertical
        for (int i = 0; i < distanceWidth; i++) {
            float beta = 2 * i + 1;

            fboDst->bind();
            globalData->distanceShader.setTexture("tex0", fboSrc->getTexture(), 0);
            globalData->distanceShader.setFloat("beta", beta);
            globalData->distanceShader.setVec2("offset", 0.0, 1.0f / (float)height);
            globalData->quad.render();

            OGL::Fbo *swap = fboDst;
            fboDst = fboSrc;
            fboSrc = swap;
        }

        // Back to AE texture
        globalData->outputFbo.bind();
        globalData->outputShader.bind();
        globalData->outputShader.setTexture("tex0", fboSrc->getTexture(), 0);
        globalData->outputShader.setFloat("multiplier16bit", multiplier16bit);
        globalData->outputShader.setFloat("width", (float)distanceWidth);
        globalData->quad.render();

        // Read pixels
        globalData->outputFbo.readToPixels(pixelsBufferP);
        ERR(AEOGLInterop::downloadTexture(pixelsBufferP, output_worldP, pixelType));

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

extern "C" DllExport PF_Err PluginDataEntryFunction(
    PF_PluginDataPtr inPtr, PF_PluginDataCB inPluginDataCallBackPtr,
    SPBasicSuite *inSPBasicSuitePtr, const char *inHostName,
    const char *inHostVersion) {
    PF_Err result = PF_Err_INVALID_CALLBACK;

    result =
        PF_REGISTER_EFFECT(inPtr, inPluginDataCallBackPtr, FX_SETTINGS_NAME,
                           FX_SETTINGS_MATCH_NAME, FX_SETTINGS_CATEGORY,
                           AE_RESERVED_INFO);  // Reserved Info

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
                break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}
