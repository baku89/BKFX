#pragma once

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace OGL {
class QuadVao {
   public:
    QuadVao();
    ~QuadVao();

    void render();

   private:
    GLuint ID = 0, quad = 0;
};

}  // namespace OGL
