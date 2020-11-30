#pragma once

#include "OGL.h"

// make sure we get 16bpc pixels;
// AE_Effect.h checks for this.
#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#define PI 3.14159265358979323864f

namespace AEOGLInterop {

void uploadTexture(OGL::RenderContext *ctx, OGL::Texture *tex,
                   PF_LayerDef *layerDef, void *pixelsBufferP) {

    GLsizei width = layerDef->width;
    GLsizei height = layerDef->height;

    GLint internalFormat = 0;
    size_t pixelBytes = 0;

    switch (ctx->format) {
    case GL_UNSIGNED_BYTE:
        internalFormat = GL_RGBA8;
        pixelBytes = sizeof(PF_Pixel8);
        break;
    case GL_UNSIGNED_SHORT:
        internalFormat = GL_RGBA16;
        pixelBytes = sizeof(PF_Pixel16);
        break;
    case GL_FLOAT:
        internalFormat = GL_RGBA32F;
        pixelBytes = sizeof(PF_PixelFloat);
        break;
    }

    // Copy to buffer per row

    char *glP = nullptr; // OpenGL
    char *aeP = nullptr; // AE

    for (size_t y = 0; y < height; y++) {
        glP = (char *)pixelsBufferP + (height - y - 1) * width * pixelBytes;
        aeP = (char *)layerDef->data + y * layerDef->rowbytes;
        std::memcpy(glP, aeP, width * pixelBytes);
    }

    OGL::setupTexture(tex, width, height, ctx->format);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, ctx->format,
                    pixelsBufferP);

    // unbind all textures
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

PF_Err downloadTexture(OGL::RenderContext *ctx, void *pixelsBufferP,
                       PF_LayerDef *layerDef) {
    
    PF_Err err = PF_Err_NONE;

    size_t pixelBytes = 0;

    switch (ctx->format) {
    case GL_UNSIGNED_BYTE:
        pixelBytes = sizeof(PF_Pixel8);
        break;
    case GL_UNSIGNED_SHORT:
        pixelBytes = sizeof(PF_Pixel16);
        break;
    case GL_FLOAT:
        pixelBytes = sizeof(PF_PixelFloat);
        break;
    }

    size_t width = layerDef->width;
    size_t height = layerDef->height;

    char *glP = nullptr; // OpenGL
    char *aeP = nullptr; // AE

    // Copy per row
    for (size_t y = 0; y < height; y++) {
        glP = (char *)pixelsBufferP + (height - y - 1) * width * pixelBytes;
        aeP = (char *)layerDef->data + y * layerDef->rowbytes;
        std::memcpy(aeP, glP, width * pixelBytes);
    }
    return err;
}

PF_Err getPointParam(PF_InData *in_data,
                      PF_OutData *out_data,
                      int paramId,
                      A_FloatPoint *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
    
    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, PARAM_CENTER, in_data->current_time,
                          in_data->time_step, in_data->time_scale,
                          &param_def));
    
    PF_PointParamSuite1 *pointSuite;
    ERR(AEFX_AcquireSuite(in_data, out_data,
                          kPFPointParamSuite,
                          kPFPointParamSuiteVersion1,
                          "Couldn't load suite.",
                          (void **)&pointSuite));
    
    ERR(pointSuite->PF_GetFloatingPointValueFromPointDef(in_data->effect_ref,
                                                         &param_def,
                                                         value));
    
    // Scale size by downsample ratio
    float width = (float)in_data->width *
       in_data->downsample_x.num / in_data->downsample_x.den;
    
    float height = (float)in_data->height *
        in_data->downsample_y.num / in_data->downsample_y.den;
    
    value->x /= width;
    value->y = 1.0f - value->y / height;
    
    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));
    
    return err;
}

PF_Err getAngleParam(PF_InData *in_data,
                     PF_OutData *out_data,
                     int paramId,
                     A_FpLong *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
    
    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale,
                          &param_def));

    PF_AngleParamSuite1 *angleSuite;
    ERR(AEFX_AcquireSuite(in_data, out_data,
        kPFAngleParamSuite,
        kPFAngleParamSuiteVersion1,
        "Couldn't load suite.",
        (void **)&angleSuite));
    
    ERR(angleSuite->PF_GetFloatingPointValueFromAngleDef(in_data->effect_ref,
                                                         &param_def,
                                                         value));
    *value = -*value * PI / 180.0f;
    
    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));
    
    return err;
}

} // namespace AEOGLInterop
