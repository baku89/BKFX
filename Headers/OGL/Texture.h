#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace OGL {
struct Texture {
    GLuint ID;
    size_t width;
    size_t height;
    GLenum format;
};

void initTexture(Texture *tex);
void setupTexture(Texture *tex, size_t width, size_t height, GLenum format);
void disposeTexture(Texture *tex);

}  // namespace OGL