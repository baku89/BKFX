#pragma once

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

namespace OGL {
GLint getInternalFormat(GLenum pixelType);

void assertOpenGLError(const std::string& msg);
}  // namespace OGL
