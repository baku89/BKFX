#pragma once

#include <memory>

#include <glm/glm.hpp>

typedef unsigned short u_int16;

#include "OGL/Common.h"
#include "OGL/GlobalContext.h"
#include "OGL/Texture.h"
#include "OGL/Shader.h"

namespace OGL {

/*
// Per render/thread supporting OpenGL variables
*/
struct RenderContext {
    int threadIndex;

    GLuint fbo = 0;
    GLuint multisampledFbo = 0;

    GLsizei width = 0;
    GLsizei height = 0;
    GLenum format = 0;

    GLuint outputTexture = 0;
    GLuint multisampledTexture = 0;

    GLuint vao = 0;
    GLuint quad = 0;

    ~RenderContext() {
        if (outputTexture) {
            glDeleteTextures(1, &outputTexture);
        }
        if (multisampledTexture) {
            glDeleteTextures(1, &multisampledTexture);
        }
        if (fbo) {
            glDeleteFramebuffers(1, &fbo);
        }
        if (vao) {
            glDeleteBuffers(1, &quad);
            glDeleteVertexArrays(1, &vao);
        }
    }
};

RenderContext *getCurrentThreadRenderContext();

void setupRenderContext(RenderContext *ctx, GLsizei width, GLsizei height,
                        GLenum format);
void setUniformTexture(RenderContext *ctx, std::string name, Texture *tex,
                       GLint index);
void setUniform1f(RenderContext *ctx, std::string name, float value);
void setUniform2f(RenderContext *ctx, std::string name, float x, float y);
void setUniformMatrix3f(RenderContext *ctx, std::string name,
                        glm::mat3x3 *value);
void renderToBuffer(RenderContext *ctx, void *pixels);
void disposeAllRenderContexts();

}  // namespace OGL
