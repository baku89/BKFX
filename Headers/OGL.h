#pragma once

#include <memory>

#include <glm/glm.hpp>

typedef unsigned short u_int16;

#include "OGL/Common.h"
#include "OGL/GlobalContext.h"
#include "OGL/Texture.h"
#include "OGL/Fbo.h"
#include "OGL/Shader.h"

namespace OGL {

/*
// Per render/thread supporting OpenGL variables
*/
struct RenderContext {
    int threadIndex;

    GLuint vao = 0;
    GLuint quad = 0;

    ~RenderContext() {
        if (vao) {
            glDeleteBuffers(1, &quad);
            glDeleteVertexArrays(1, &vao);
        }
    }
};

RenderContext *getCurrentThreadRenderContext();

void setupRenderContext(RenderContext *ctx, GLsizei width, GLsizei height,
                        GLenum format);
void render(RenderContext *ctx);
void disposeAllRenderContexts();

}  // namespace OGL
