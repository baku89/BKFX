#include "Common.h"
#include "Texture.h"

namespace OGL {

void Texture::allocate(size_t width, size_t height, GLenum format) {
    bool sizeChanged = this->width != width || this->height != height;
    bool formatChanged = this->format != format;

    this->width = width;
    this->height = height;
    this->format = format;

    if (sizeChanged || formatChanged) {
        glDeleteTextures(1, &this->ID);
        this->ID = 0;
    }

    if (this->ID == 0) {
        GLint internalFormat = OGL::getInternalFormat(this->format);

        glGenTextures(1, &this->ID);
        glBindTexture(GL_TEXTURE_2D, this->ID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, this->width, this->height,
                     0, GL_RGBA, this->format, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, this->width);
}

Texture::~Texture() {
    glDeleteTextures(1, &this->ID);
}
}  // namespace OGL
