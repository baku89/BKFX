#include <glad/glad.h>

#include "Texture.h"

namespace OGL {

class Fbo {
   public:
    Fbo();
    ~Fbo();

    void allocate(GLsizei width, GLsizei height, GLuint pixelType, int numSamples = 0);
    void bind();
    void unbind();
    Texture* getTexture();
    void readToPixels(void* pixels);

   private:
    GLuint ID = 0, multisampledFbo = 0, multisampledTexture = 0;
    GLsizei width = 0, height = 0, numSamples = 0;
    GLenum pixelType = -1;
    Texture texture;
};

}  // namespace OGL