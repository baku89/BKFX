#include "OGL.h"

#include "Debug.h"

#include <atomic>

#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

static const struct {
    float x, y;
} quadVertices[4] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};

GLuint createQuadVBO() {
    // Setup VertexBufferObjevt
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    return vbo;
}

GLuint createQuadVAO(GLuint vbo) {
    // Setup VertexArrayObject
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    return vao;
}

/**
 * OGL
 */

namespace OGL {

bool globalSetup(GlobalContext *ctx) {
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(640, 480, "", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        return false;
    }

    // Setup Shader
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        FX_LOG("Failed to initialize GLAD");
        return false;
    }
    //
    ctx->window = window;

    return true;
}

void makeGlobalContextCurrent(GlobalContext *ctx) {
    glfwMakeContextCurrent(ctx->window);
}

bool globalSetdown(GlobalContext *ctx) {
    glfwDestroyWindow(ctx->window);
    glfwTerminate();

    return true;
}

/**
  RenderContext
 */
typedef std::shared_ptr<RenderContext> RenderContextPtr;

thread_local int threadIndex = -1;
std::atomic_int threadCounter;

std::map<int, RenderContextPtr> renderContextMap;
std::recursive_mutex renderContextMutex;

#define MUTEX_LOCK \
    std::lock_guard<std::recursive_mutex> func_locker(renderContextMutex)

RenderContext *getCurrentThreadRenderContext() {
    MUTEX_LOCK;

    RenderContextPtr result;

    if (threadIndex == -1) {
        threadIndex = threadCounter++;

        result.reset(new RenderContext());
        result->threadIndex = threadIndex;

        FX_LOG("getCurrentThreadRenderContext: RenderContext index="
               << threadIndex << " initialized");

        renderContextMap[threadIndex] = result;
    } else {
        result = renderContextMap[threadIndex];
    }
    return result.get();
}

void setupRenderContext(RenderContext *ctx, GLsizei width, GLsizei height, GLenum format) {
    bool sizeChanged = ctx->width != width || ctx->height != height;
    bool formatChanged = ctx->format != format;

    ctx->width = width;
    ctx->height = height;
    ctx->format = format;

    if (sizeChanged || formatChanged) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // release framebuffer resources
        if (ctx->fbo) {
            glDeleteFramebuffers(1, &ctx->fbo);
            ctx->fbo = 0;
        }
        if (ctx->multisampledFbo) {
            glDeleteFramebuffers(1, &ctx->multisampledFbo);
            ctx->multisampledFbo = 0;
        }
        if (ctx->outputTexture) {
            glDeleteTextures(1, &ctx->outputTexture);
            ctx->outputTexture = 0;
        }
        if (ctx->multisampledTexture) {
            glDeleteTextures(1, &ctx->multisampledTexture);
            ctx->multisampledTexture = 0;
        }
    }

    // Create bufffers
    if (ctx->vao == 0) {
        ctx->quad = createQuadVBO();
        ctx->vao = createQuadVAO(ctx->quad);
    }

    // Create a frame-buffer object
    if (ctx->fbo == 0) {
        glGenFramebuffers(1, &ctx->fbo);
    }
    if (ctx->multisampledFbo == 0) {
        glGenFramebuffers(1, &ctx->multisampledFbo);
    }

    // GLator effect specific OpenGL resource loading
    // create an empty texture for the input surface
    if (ctx->multisampledTexture == 0) {
        GLint internalFormat = OGL::getInternalFormat(format);

        glGenTextures(1, &ctx->multisampledTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ctx->multisampledTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, internalFormat,
                                ctx->width, ctx->height, GL_TRUE);

        // Bind to fbo
        glBindFramebuffer(GL_FRAMEBUFFER, ctx->multisampledFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                               ctx->multisampledTexture, 0);
    }

    if (ctx->outputTexture == 0) {
        GLint internalFormat = getInternalFormat(format);

        glGenTextures(1, &ctx->outputTexture);
        glBindTexture(GL_TEXTURE_2D, ctx->outputTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, ctx->width, ctx->height,
                     0, GL_RGBA, ctx->format, nullptr);

        // // Bind to fbo
        glBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               ctx->outputTexture, 0);
    }

    // makeReadyToRender

    // Link attribute location
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(quadVertices[0]),
                          nullptr);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers

    // Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->multisampledFbo);
    glViewport(0, 0, ctx->width, ctx->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void renderToBuffer(RenderContext *ctx, void *pixels) {
    // Render and flush
    glBindVertexArray(ctx->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Bind the multisampled FBO for reading
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->multisampledFbo);
    // Bind the normal FBO for drawing
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->fbo);
    // Blit the multisampled FBO to the normal FBO
    glBlitFramebuffer(0, 0, ctx->width, ctx->height, 0, 0, ctx->width, ctx->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    // Bing the normal FBO for reading
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo);
    // Read Ppxels
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, ctx->format, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void disposeAllRenderContexts() {
    MUTEX_LOCK;
    renderContextMap.clear();
}

};  // namespace OGL
