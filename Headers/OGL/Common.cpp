#include "Common.h"

namespace OGL {

GLint getInternalFormat(GLenum pixelType) {
    switch (pixelType) {
        case GL_UNSIGNED_BYTE:
            return GL_RGBA8;
        case GL_UNSIGNED_SHORT:
            return GL_RGBA16UI;
        case GL_FLOAT:
            return GL_RGBA32F;
    }
    return 0;
}
}  // namespace OGL
