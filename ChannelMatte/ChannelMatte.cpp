#include "ChannelMatte.h"
#include "AEFX_SuiteHelper.h"
#include "Smart_Utils.h"

#include "Settings.h"

static PF_Err 
About (	
    PF_InData		*in_data,
    PF_OutData		*out_data,
    PF_ParamDef		*params[],
    PF_LayerDef		*output )
{
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
                                          "%s v%d.%d\r%s",
                                          FX_SETTINGS_NAME,
                                          MAJOR_VERSION,
                                          MINOR_VERSION,
                                          FX_SETTINGS_DESCRIPTION);
    return PF_Err_NONE;
}

static PF_Err GlobalSetup(
    PF_InData    *in_data,
    PF_OutData   *out_data,
    PF_ParamDef  *params[],
    PF_LayerDef  *output) {

    out_data->my_version = PF_VERSION(MAJOR_VERSION, 
                                      MINOR_VERSION,
                                      BUG_VERSION, 
                                      STAGE_VERSION, 
                                      BUILD_VERSION);

    // Enable 16bpc
    out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;
    
    // Enable 32bpc
    out_data->out_flags2 =
        PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_SUPPORTS_SMART_RENDER;
    
    return PF_Err_NONE;
}

static PF_Err 
ParamsSetup(
    PF_InData		*in_data,
    PF_OutData		*out_data,
    PF_ParamDef		*params[],
    PF_LayerDef		*output) {

    PF_Err		err		= PF_Err_NONE;

    PF_ParamDef	def;	

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Source Channel",
                 4,
                 4,
                 "Red|Green|Blue|Alpha",
                 PARAM_SOURCE_CHANNEL);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Matte Type",
                 2,
                 1,
                 "Luma|Alpha",
                 PARAM_MATTE_TYPE);
    
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX(
        "Invert",
        "", // Description
        FALSE,
        0,
        PARAM_INVERT);

    out_data->num_params = PARAM_NUM_PARAMS;

    return err;
}

static PF_Err GlobalSetdown(PF_InData *in_data, PF_OutData *out_data,
                            PF_ParamDef *params[], PF_LayerDef *output) {
    
    PF_Err err = PF_Err_NONE;
    return err;
}

static PF_Err
PixelIteratorFunc8 (
    void        *refcon,
    A_long        xL,
    A_long        yL,
    PF_Pixel8    *inP,
    PF_Pixel8    *outP)
{
    PF_Err       err = PF_Err_NONE;

    ParamInfo    *paramInfo    = reinterpret_cast<ParamInfo*>(refcon);
                    
    if (paramInfo) {
        
        A_u_char val = 0;
        
        switch (paramInfo->sourceChannel) {
            case 1: val = inP->red;     break;
            case 2: val = inP->green;   break;
            case 3: val = inP->blue;    break;
            case 4: val = inP->alpha;   break;
        }
        
        if (paramInfo->invert) {
            val = PF_MAX_CHAN8 - val;
        }
        
        if (paramInfo->matteType == 1) {
            // Luma
            outP->alpha = PF_MAX_CHAN8;
            outP->red   = val;
            outP->green = val;
            outP->blue  = val;
        } else {
            // Alpha Matte filled with white
            outP->alpha = val;
            outP->red   = PF_MAX_CHAN8;
            outP->green = PF_MAX_CHAN8;
            outP->blue  = PF_MAX_CHAN8;
        }
    }

    return err;
}

static PF_Err
PixelIteratorFunc16 (
    void		*refcon, 
    A_long		xL, 
    A_long		yL, 
    PF_Pixel16	*inP, 
    PF_Pixel16	*outP)
{
    PF_Err       err = PF_Err_NONE;

    ParamInfo    *paramInfo    = reinterpret_cast<ParamInfo*>(refcon);
                    
    if (paramInfo) {
        
        A_u_short val = 0;
        
        switch (paramInfo->sourceChannel) {
            case 1: val = inP->red;     break;
            case 2: val = inP->green;   break;
            case 3: val = inP->blue;    break;
            case 4: val = inP->alpha;   break;
        }
        
        if (paramInfo->invert) {
            val = PF_MAX_CHAN16 - val;
        }
        
        if (paramInfo->matteType == 1) {
            // Luma
            outP->alpha = PF_MAX_CHAN16;
            outP->red   = val;
            outP->green = val;
            outP->blue  = val;
        } else {
            // Alpha Matte filled with white
            outP->alpha = val;
            outP->red   = PF_MAX_CHAN16;
            outP->green = PF_MAX_CHAN16;
            outP->blue  = PF_MAX_CHAN16;
        }
    }

    return err;
}

static PF_Err
PixelIteratorFunc32 (
    void        *refcon,
    A_long        xL,
    A_long        yL,
    PF_PixelFloat *inP,
    PF_PixelFloat *outP)
{ 
    PF_Err       err = PF_Err_NONE;

    ParamInfo    *paramInfo    = reinterpret_cast<ParamInfo*>(refcon);
                    
    if (paramInfo) {
        
        PF_FpShort val = 0;
        
        switch (paramInfo->sourceChannel) {
            case 1: val = inP->red;     break;
            case 2: val = inP->green;   break;
            case 3: val = inP->blue;    break;
            case 4: val = inP->alpha;   break;
        }
        
        if (paramInfo->invert) {
            val = PF_MAX_CHAN32 - val;
        }
        
        if (paramInfo->matteType == PF_MAX_CHAN32) {
            // Luma
            outP->alpha = PF_MAX_CHAN32;
            outP->red   = val;
            outP->green = val;
            outP->blue  = val;
        } else {
            // Alpha Matte filled with white
            outP->alpha = val;
            outP->red   = PF_MAX_CHAN32;
            outP->green = PF_MAX_CHAN32;
            outP->blue  = PF_MAX_CHAN32;
        }
    }

    return err;
}

static PF_Err PreRender(PF_InData *in_data, PF_OutData *out_data,
                        PF_PreRenderExtra *extra) {
    PF_Err err = PF_Err_NONE;


    PF_RenderRequest req = extra->input->output_request;
    PF_CheckoutResult in_result;

    // Create paramInfo
    AEFX_SuiteScoper<PF_HandleSuite1> handleSuite
        = AEFX_SuiteScoper<PF_HandleSuite1>(in_data,
                                            kPFHandleSuite,
                                            kPFHandleSuiteVersion1,
                                            out_data);

    PF_Handle paramInfoH = handleSuite->host_new_handle(sizeof(ParamInfo));
    
    if (!paramInfoH) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    ParamInfo *paramInfo = reinterpret_cast<ParamInfo*>(handleSuite->host_lock_handle(paramInfoH));
    
    if (!paramInfo) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    // Set handler
    extra->output->pre_render_data = paramInfoH;
    
    // Checkout Params
    PF_ParamDef source_channel_param;
    AEFX_CLR_STRUCT(source_channel_param);
    ERR(PF_CHECKOUT_PARAM(in_data,
                          PARAM_SOURCE_CHANNEL,
                          in_data->current_time,
                          in_data->time_step,
                          in_data->time_scale,
                          &source_channel_param));
    
    PF_ParamDef matte_type_param;
    AEFX_CLR_STRUCT(matte_type_param);
    ERR(PF_CHECKOUT_PARAM(in_data,
                          PARAM_MATTE_TYPE,
                          in_data->current_time,
                          in_data->time_step,
                          in_data->time_scale,
                          &matte_type_param));

    PF_ParamDef invert_param;
    AEFX_CLR_STRUCT(invert_param);
    ERR(PF_CHECKOUT_PARAM(in_data,
                          PARAM_INVERT,
                          in_data->current_time,
                          in_data->time_step,
                          in_data->time_scale,
                          &invert_param));
    
    // Assign latest param values
    if (!err) {
        paramInfo->sourceChannel = source_channel_param.u.pd.value;
        paramInfo->matteType = matte_type_param.u.pd.value;
        paramInfo->invert = invert_param.u.bd.value;
    }
    
    handleSuite->host_unlock_handle(paramInfoH);
    
    // Checkout Input Image
    ERR(extra->cb->checkout_layer(in_data->effect_ref, PARAM_INPUT,
                                  PARAM_INPUT, &req, in_data->current_time,
                                  in_data->time_step, in_data->time_scale,
                                  &in_result));
    
    // Compute rendering rect
    if (!err) {
        UnionLRect(&in_result.result_rect, &extra->output->result_rect);
        UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);
    }
    
    return err;
}

static PF_Err SmartRender(PF_InData           *in_data,
                          PF_OutData          *out_data,
                          PF_SmartRenderExtra *extra) {
    
    PF_Err                err        = PF_Err_NONE,
                          err2       = PF_Err_NONE;
    
    PF_EffectWorld *input_worldP = NULL,
                   *output_worldP = NULL;
    
    PF_WorldSuite2 *wsP = NULL;
    
    PF_PixelFormat format = PF_PixelFormat_INVALID;
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    // Retrieve paramInfo
    AEFX_SuiteScoper<PF_HandleSuite1> handleSuite =
        AEFX_SuiteScoper<PF_HandleSuite1>(in_data,
                                          kPFHandleSuite,
                                          kPFHandleSuiteVersion1,
                                          out_data);
    ParamInfo *paramInfo = reinterpret_cast<ParamInfo*>(handleSuite->host_lock_handle(reinterpret_cast<PF_Handle>(extra->input->pre_render_data)));
    
    // Checkout layer pixels
    ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, PARAM_INPUT,
                                          &input_worldP)));
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));
    
    // Setup wsP
    ERR(AEFX_AcquireSuite(in_data,
                          out_data,
                          kPFWorldSuite,
                          kPFWorldSuiteVersion2,
                          "Couldn't load suite.",
                          (void **)&wsP));
    
    // Get format
    ERR(wsP->PF_GetPixelFormat(input_worldP, &format));
    
    switch (format) {
        case PF_PixelFormat_ARGB32: // 8bpc
            ERR(suites.Iterate8Suite1()->iterate(in_data,
                                                 // progress base
                                                 0,
                                                 // progress final
                                                 input_worldP->height,
                                                 // src
                                                 input_worldP,
                                                 // area - null for all pixels
                                                 NULL,
                                                 // refcon - your custom data pointer
                                                 (void*)paramInfo,
                                                 // pixel function pointer
                                                 PixelIteratorFunc8,
                                                 output_worldP));
            break;
        case PF_PixelFormat_ARGB64: // 16bpc
            ERR(suites.Iterate16Suite1()->iterate(in_data,
                                                  0,
                                                  input_worldP->height,
                                                  input_worldP,
                                                  NULL,
                                                  (void*)paramInfo,
                                                  PixelIteratorFunc16,
                                                  output_worldP));
            break;
        case PF_PixelFormat_ARGB128: // 32bpc
            ERR(suites.IterateFloatSuite1()->iterate(in_data,
                                                     0,
                                                     input_worldP->height,
                                                     input_worldP,
                                                     NULL,
                                                     (void*)paramInfo,
                                                     PixelIteratorFunc32,
                                                     output_worldP));
            break;
        default:
            break;
    }
    
    // Check in
    ERR2(AEFX_ReleaseSuite(in_data, out_data, kPFWorldSuite,
                           kPFWorldSuiteVersion2, "Couldn't release suite."));
    
//    ERR2(PF_CHECKIN_PARAM(in_data, &gain_param));
    
    ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, PARAM_INPUT));

    return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
    PF_PluginDataPtr inPtr,
    PF_PluginDataCB inPluginDataCallBackPtr,
    SPBasicSuite* inSPBasicSuitePtr,
    const char* inHostName,
    const char* inHostVersion)
{
    PF_Err result = PF_Err_INVALID_CALLBACK;

    result = PF_REGISTER_EFFECT(inPtr,
                                inPluginDataCallBackPtr,
                                FX_SETTINGS_NAME,
                                FX_SETTINGS_MATCH_NAME,
                                FX_SETTINGS_CATEGORY,
                                AE_RESERVED_INFO); // Reserved Info

    return result;
}


PF_Err
EffectMain(
    PF_Cmd			cmd,
    PF_InData		*in_data,
    PF_OutData		*out_data,
    PF_ParamDef		*params[],
    PF_LayerDef		*output,
    void			*extra)
{
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
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}
