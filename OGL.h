#pragma once

#include <memory>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

typedef unsigned short u_int16;

namespace OGL {

/*
// Texture
*/
struct Texture {
    GLuint texid;
    size_t width;
    size_t height;
    GLenum format;
};

void initTexture(Texture *tex);

void setupTexture(Texture *tex, size_t width, size_t height, GLenum format);

void disposeTexture(Texture *tex);

/*
// Global (to the effect) supporting OpenGL contexts
*/
struct GlobalContext {
    GLFWwindow *window;
};
bool globalSetup(GlobalContext *ctx);
void makeGlobalContextCurrent(GlobalContext *ctx);
bool globalSetdown(GlobalContext *ctx);

/*
// Per render/thread supporting OpenGL variables
*/
struct RenderContext {

    int threadIndex;

    GLuint frameBuffer = 0;

    GLsizei width = 0;
    GLsizei height = 0;
    GLenum format = 0;

    GLuint program = 0;
    GLuint outputTexture = 0;

    GLuint vao = 0;
    GLuint quad = 0;

    ~RenderContext() {
        if (outputTexture) {
            glDeleteTextures(1, &outputTexture);
        }
        if (program) {
            glDeleteProgram(program);
        }
        if (frameBuffer) {
            glDeleteFramebuffers(1, &frameBuffer);
        }
        if (vao) {
            glDeleteBuffers(1, &quad);
            glDeleteVertexArrays(1, &vao);
        }
    }
};

RenderContext *getCurrentThreadRenderContext();

void setupRenderContext(RenderContext *ctx, GLsizei width, GLsizei height,
                        GLenum format, std::string vertShaderPath,
                        std::string fragShaderPath);
void setUniformTexture(RenderContext *ctx, std::string name, Texture *tex,
                       GLint index);
void setUniform1f(RenderContext *ctx, std::string name, float value);
void setUniform2f(RenderContext *ctx, std::string name, float x, float y);
void renderToBuffer(RenderContext *ctx, void *pixels);
void disposeAllRenderContexts();

} // namespace OGL
