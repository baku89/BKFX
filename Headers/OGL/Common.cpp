#include "Common.h"

#include "Debug.h"

#include <sstream>

namespace OGL {

GLint getInternalFormat(GLenum pixelType) {
    switch (pixelType) {
        case GL_UNSIGNED_BYTE:
            return GL_RGBA8;
        case GL_UNSIGNED_SHORT:
            return GL_RGBA16_EXT;
        case GL_FLOAT:
            return GL_RGBA32F;
    }
    return 0;
}

void assertOpenGLError(const std::string& msg) {
    GLenum error = glGetError();

    if (error != GL_NO_ERROR) {
        std::stringstream s;
        s << "OpenGL error ";

        switch (error) {
            case GL_INVALID_ENUM:
                s << "GL_INVALID_ENUM ";
                break;
            case GL_INVALID_VALUE:
                s << "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                s << "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                s << "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                s << "GL_OUT_OF_MEMORY";
                break;
        }

        s << " (0x" << std::hex << error << ") at " << msg;
        FX_LOG(s.str());
    }
}

}  // namespace OGL
