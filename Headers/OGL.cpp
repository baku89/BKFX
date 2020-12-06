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

void setupRenderContext(RenderContext *ctx, GLsizei width, GLsizei height, GLenum pixelType) {
    // Create bufffers
    if (ctx->vao == 0) {
        ctx->quad = createQuadVBO();
        ctx->vao = createQuadVAO(ctx->quad);
    }

    // Link attribute location
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(quadVertices[0]),
                          nullptr);
}

void render(RenderContext *ctx) {
    // Render and flush
    glBindVertexArray(ctx->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void disposeAllRenderContexts() {
    MUTEX_LOCK;
    renderContextMap.clear();
}

};  // namespace OGL
