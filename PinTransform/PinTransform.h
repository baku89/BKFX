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
#include "AEFX_SuiteHelper.h"
#include "Smart_Utils.h"

#include "../OGL.h"

#include <glm/glm.hpp>

/* Other useful constants */
#define PF_MAX_CHAN32 1.0f

/* Versioning information */

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

/* Parameter defaults */

#define SLIDER_PRECISION 1

enum { PARAM_EDITING_MODE_SRC = 1,
       PARAM_EDITING_MODE_DST,
       PARAM_EDITING_MODE_BOTH };

enum {
    PARAM_INPUT = 0,
    PARAM_EDITING_MODE,
    PARAM_PINCOUNT,
    PARAM_SRC_1,
    PARAM_SRC_2,
    PARAM_SRC_3,
    PARAM_SRC_4,
    PARAM_DST_1,
    PARAM_DST_2,
    PARAM_DST_3,
    PARAM_DST_4,
    PARAM_COPY_SRC_TO_DST,
    PARAM_SWAP_SRC_DST,
    PARAM_NUM_PARAMS
};

struct GlobalData {
    OGL::GlobalContext globalContext;
    OGL::Texture inputTexture;
    OGL::Shader program;
    OGL::Fbo fbo;
    OGL::QuadVao quad;
};

struct ParamInfo {
    A_long pinCount;
    glm::mat3x3 xform;
};

extern "C" {

DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData *in_data,
                            PF_OutData *out_data, PF_ParamDef *params[],
                            PF_LayerDef *output, void *extra);
}
