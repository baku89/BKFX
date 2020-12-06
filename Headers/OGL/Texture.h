#pragma once

#include <glad/glad.h>

namespace OGL {
class Texture {
   public:
    ~Texture();

    void allocate(GLsizei width, GLsizei height, GLenum pixelType);
    void bind();
    void unbind();

   private:
    GLuint ID = 0;
    size_t width = 0;
    size_t height = 0;
    GLenum pixelType = 0;
};

}  // namespace OGL