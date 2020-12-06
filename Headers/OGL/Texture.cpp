#include "Common.h"
#include "Texture.h"

namespace OGL {

void Texture::allocate(GLsizei width, GLsizei height, GLenum pixelType) {
    bool configChanged = this->width != width || this->height != height;
    configChanged |= this->pixelType != pixelType;

    this->width = width;
    this->height = height;
    this->pixelType = pixelType;

    if (configChanged) {
        glDeleteTextures(1, &this->ID);
        this->ID = 0;
    }

    if (this->ID == 0) {
        GLint internalFormat = OGL::getInternalFormat(this->pixelType);

        glGenTextures(1, &this->ID);
        glBindTexture(GL_TEXTURE_2D, this->ID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, this->width, this->height,
                     0, GL_RGBA, this->pixelType, nullptr);
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

void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, this->ID);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace OGL
