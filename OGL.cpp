#include "OGL.h"

#include "Settings.h"

#include <atomic>
#include <fstream>

#include <map>
#include <mutex>
#include <thread>
#include <vector>

static const struct {
    float x, y;
} quadVertices[4] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};

GLint getInternalFormat(GLenum format) {
    switch (format) {
        case GL_UNSIGNED_BYTE:
            return GL_RGBA8;
        case GL_UNSIGNED_SHORT:
            return GL_RGBA16;
        case GL_FLOAT:
            return GL_RGBA32F;
    }
    return 0;
}

std::string readTextFile(std::string inFilename) {
    std::ifstream ifs(inFilename.c_str());
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
}

bool checkShaderCompiled(GLuint shader) {
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
        FX_LOG("Shader error = " << &infoLog[0]);

        return false;
    } else {
        return true;
    }
}

bool checkProgramLinked(GLuint program) {
    GLint isLinked = 0;

    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
        FX_LOG("Program error = " << &infoLog[0]);

        return false;
    } else {
        return true;
    }
}

GLuint initShader(std::string vertPath, std::string fragPath) {
    FX_LOG("Load shader file: vert='" << vertPath << "' frag='" << fragPath);

    // load shader text
    std::string vertCode = readTextFile(vertPath);
    std::string fragCode = readTextFile(fragPath);

    const char *vertStrP = vertCode.c_str();
    const char *fragStrP = fragCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertStrP, nullptr);
    glCompileShader(vertexShader);

    // Check for compilation result
    if (!checkShaderCompiled(vertexShader)) {
        return -1;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragStrP, nullptr);
    glCompileShader(fragmentShader);

    if (!checkShaderCompiled(fragmentShader)) {
        return -1;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    if (!checkProgramLinked(program)) {
        return -1;
    }

    return program;
}

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

void initTexture(Texture *tex) {
    tex->texid = 0;
    tex->width = 0;
    tex->height = 0;
    tex->format = 0;
}

void setupTexture(Texture *tex, size_t width, size_t height, GLenum format) {
    bool sizeChanged = tex->width != width || tex->height != height;
    bool formatChanged = tex->format != format;

    tex->width = width;
    tex->height = height;
    tex->format = format;

    if (sizeChanged || formatChanged) {
        glDeleteTextures(1, &tex->texid);
        tex->texid = 0;
    }

    if (tex->texid == 0) {
        GLint internalFormat = getInternalFormat(tex->format);

        glGenTextures(1, &tex->texid);
        glBindTexture(GL_TEXTURE_2D, tex->texid);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tex->width, tex->height,
                     0, GL_RGBA, tex->format, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
}

void disposeTexture(Texture *tex) { glDeleteTextures(1, &tex->texid); }

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

void setupRenderContext(RenderContext *ctx, GLsizei width, GLsizei height,
                        GLenum format, std::string vertShaderPath,
                        std::string fragShaderPath) {
    bool sizeChanged = ctx->width != width || ctx->height != height;
    bool formatChanged = ctx->format != format;

    ctx->width = width;
    ctx->height = height;
    ctx->format = format;

    if (sizeChanged || formatChanged) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // release framebuffer resources
        if (ctx->frameBuffer) {
            glDeleteFramebuffers(1, &ctx->frameBuffer);
            ctx->frameBuffer = 0;
        }
        if (ctx->outputTexture) {
            glDeleteTextures(1, &ctx->outputTexture);
            ctx->outputTexture = 0;
        }
    }

    // Create bufffers
    if (ctx->vao == 0) {
        ctx->quad = createQuadVBO();
        ctx->vao = createQuadVAO(ctx->quad);
    }

    // Create a frame-buffer object and bind it...
    if (ctx->frameBuffer == 0) {
        // create the color buffer to render to
        glGenFramebuffers(1, &ctx->frameBuffer);
    }

    // GLator effect specific OpenGL resource loading
    // create an empty texture for the input surface
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
    }

    if (ctx->program == 0) {
        // initialize and compile the shader objects
        ctx->program = initShader(vertShaderPath, fragShaderPath);
    }

    // makeReadyToRender

    // Link attribute location
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(quadVertices[0]),
                          nullptr);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers

    // Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           ctx->outputTexture, 0);

    glViewport(0, 0, ctx->width, ctx->height);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(ctx->program);
}

void setUniformTexture(RenderContext *ctx, std::string name, Texture *tex,
                       GLint index) {
    if (tex) {
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(GL_TEXTURE_2D, tex->texid);

        GLuint location = glGetUniformLocation(ctx->program, name.c_str());
        glUniform1i(location, index);
    }
}

void setUniform1f(RenderContext *ctx, std::string name, float value) {
    GLuint location = glGetUniformLocation(ctx->program, name.c_str());
    glUniform1f(location, value);
}

void setUniform2f(RenderContext *ctx, std::string name, float x, float y) {
    GLuint location = glGetUniformLocation(ctx->program, name.c_str());
    glUniform2f(location, x, y);
}

void renderToBuffer(RenderContext *ctx, void *pixels) {
    // Render and flush
    glBindVertexArray(ctx->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);

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
