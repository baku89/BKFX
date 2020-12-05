#include "Common.h"
#include "Texture.h"

namespace OGL {
void initTexture(Texture *tex) {
    tex->ID = 0;
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
        glDeleteTextures(1, &tex->ID);
        tex->ID = 0;
    }

    if (tex->ID == 0) {
        GLint internalFormat = OGL::getInternalFormat(tex->format);

        glGenTextures(1, &tex->ID);
        glBindTexture(GL_TEXTURE_2D, tex->ID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tex->width, tex->height,
                     0, GL_RGBA, tex->format, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
}

void disposeTexture(Texture *tex) { glDeleteTextures(1, &tex->ID); }
}  // namespace OGL