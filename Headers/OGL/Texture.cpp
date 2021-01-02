#include "Common.h"
#include "Texture.h"

namespace OGL {

void Texture::allocate(GLsizei width, GLsizei height,
                       GLenum format, GLenum pixelType) {
    glGetError();

    bool configChanged = this->width != width || this->height != height;
    configChanged |= this->format != format;
    configChanged |= this->pixelType != pixelType;

    this->width = width;
    this->height = height;
    this->format = format;
    this->pixelType = pixelType;

    if (configChanged) {
        glDeleteTextures(1, &this->ID);
        assertOpenGLError("glDeleteTexture");
        this->ID = 0;
    }

    if (this->ID == 0) {
        glGenTextures(1, &this->ID);

        this->bind();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        assertOpenGLError("glTexParameteri");

        glTexImage2D(GL_TEXTURE_2D, 0, format, this->width, this->height,
                     0, format, this->pixelType, nullptr);
        assertOpenGLError("glTexImage2D");

        this->unbind();
    }
}

Texture::~Texture() {
    glDeleteTextures(1, &this->ID);
}

void Texture::bind() {
    glGetError();
    glBindTexture(GL_TEXTURE_2D, this->ID);
    assertOpenGLError("glBindTexture");
}

void Texture::unbind() {
    glGetError();
    glBindTexture(GL_TEXTURE_2D, 0);
    assertOpenGLError("glUnbindTexture");
}

GLuint Texture::getID() {
    return this->ID;
}

}  // namespace OGL
