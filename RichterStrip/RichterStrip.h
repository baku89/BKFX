#pragma once

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned short u_int16;
typedef unsigned long u_long;
typedef short int int16;
#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096

// make sure we get 16bpc pixels;
// AE_Effect.h checks for this.
#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#ifdef AE_OS_WIN
typedef unsigned short PixelType;
#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "OGL.h"

/* Other useful constants */
#define PF_MAX_CHAN32 1.0f

/* Versioning information */

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

/* Parameter defaults */

enum { PARAM_INPUT = 0,
       PARAM_CENTER,
       PARAM_ANGLE,
       PARAM_NUM_PARAMS };

struct GlobalData {
    OGL::GlobalContext globalContext;
    OGL::Texture inputTexture;
    OGL::Shader program;
};

struct ParamInfo {
    A_FloatPoint center;
    A_FpLong angle;
};

extern "C" {

DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData *in_data,
                            PF_OutData *out_data, PF_ParamDef *params[],
                            PF_LayerDef *output, void *extra);
}
