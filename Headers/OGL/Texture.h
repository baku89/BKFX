#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace OGL {
class Texture {
   public:
    GLuint ID = 0;
    size_t width = 0;
    size_t height = 0;
    GLenum pixelType = 0;

    void allocate(size_t width, size_t height, GLenum pixelType);
    ~Texture();
};

}  // namespace OGL