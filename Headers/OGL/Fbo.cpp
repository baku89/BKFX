#include "Debug.h"

#include "Common.h"

#include "Fbo.h"
#include <iostream>
#include <sstream>

namespace OGL {

Fbo::Fbo() : texture() {}

Fbo::~Fbo() {
    if (this->ID) {
        glDeleteFramebuffers(1, &this->ID);
    }
    if (this->rbo) {
        glDeleteRenderbuffers(1, &this->rbo);
    }
    //    this->texture.~Texture();
    if (this->multisampledFbo) {
        glDeleteFramebuffers(1, &this->multisampledFbo);
        assertOpenGLError("glDeleteFramebuffers");
    }
    if (this->multisampledTexture) {
        glDeleteTextures(1, &this->multisampledTexture);
        assertOpenGLError("glDeleteTextures");
    }
}

void Fbo::allocate(GLsizei width, GLsizei height, GLenum format, GLenum pixelType, int numSamples) {
    glGetError();

    bool configChanged = this->width != width || this->height != height;
    configChanged |= this->pixelType != pixelType;
    configChanged |= this->numSamples != numSamples;

    this->width = width;
    this->height = height;
    this->format = format;
    this->pixelType = pixelType;
    this->numSamples = numSamples;

    if (configChanged) {
        glDeleteFramebuffers(1, &this->ID);
        assertOpenGLError("Fbo::allocate glDeleteFrameBuffers");
        this->ID = 0;
    }

    if (this->ID == 0) {
        glGenFramebuffers(1, &this->ID);
        assertOpenGLError("Fbo::allocate glGenFrameBuffers");

        glBindFramebuffer(GL_FRAMEBUFFER, this->ID);
        assertOpenGLError("Fbo::allocate glBindFramebuffer");

        // Attach render buffer to fbo
        GLenum rboFormat;
        switch (pixelType) {
            case GL_UNSIGNED_BYTE:
                rboFormat = GL_RGBA8;
                break;
            case GL_UNSIGNED_SHORT:
                // NOTE: Not yet supported
                rboFormat = GL_RGB16_EXT;
                break;
            case GL_FLOAT:
                rboFormat = GL_RGBA32F_EXT;
                break;
        }

        glGenRenderbuffers(1, &this->rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, this->rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, rboFormat, width, height);
        assertOpenGLError("Fbo::allocate glRenderbufferStorage");

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
        assertOpenGLError("Fbo::allocate glFramebufferRenderbuffer");

        // Attach texture to fbo
        //        this->texture.allocate(width, height, format, pixelType);
        //        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        //                               this->texture.getID(), 0);
        //        assertOpenGLError("Fbo::glFramebufferTexture2D");

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            std::stringstream s;
            s << "FrameBuffer incomplete: 0x" << std::hex << error;
            FX_LOG(s.str());
        }

        // Initialize multisampled FBO/texture
        //        if (this->numSamples > 0) {
        //            // Geneate FBo
        //            glGenFramebuffers(1, &this->multisampledFbo);
        //            glBindFramebuffer(GL_FRAMEBUFFER, this->multisampledFbo);
        //
        //            // Generate multisampled texture
        //            glGenTextures(1, &this->multisampledTexture);
        //            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->multisampledTexture);
        //
        //            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //
        //
        //            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, this->numSamples, format,
        //                                    this->width, this->height, GL_TRUE);
        //
        //            // Bind the texture to FBo
        //            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
        //                                   this->multisampledTexture, 0);
        //        }
    }
}

void Fbo::bind() {
    glGetError();
    GLuint fbo = this->numSamples > 0 ? this->multisampledFbo : this->ID;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    assertOpenGLError("Fbo::glBindFramebuffer");

    glViewport(0, 0, this->width, this->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Fbo::unbind() {
    glGetError();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    assertOpenGLError("Fbo::glUnbindFramebuffer");
}

//Texture* Fbo::getTexture() {
//    if (this->numSamples > 0) {
//        // Bind the multisampled FBO for reading
//        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multisampledFbo);
//        // Bind the normal FBO for drawing
//        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ID);
//        // Blit the multisampled FBO to the normal FBO
//        glBlitFramebuffer(0, 0, this->width, this->height,
//                          0, 0, this->width, this->height,
//                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
//    }
//
//    return &this->texture;
//}

void Fbo::readToPixels(void* pixels) {
    glGetError();
    //    if (this->numSamples > 0) {
    //        // Bind the multisampled FBO for reading
    //        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multisampledFbo);
    //        // Bind the normal FBO for drawing
    //        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ID);
    //        // Blit the multisampled FBO to the normal FBO
    //        glBlitFramebuffer(0, 0, this->width, this->height,
    //                          0, 0, this->width, this->height,
    //                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    //    }

    // Bing the normal FBO for reading
    glBindFramebuffer(GL_FRAMEBUFFER, this->ID);

    assertOpenGLError("Fbo::readToPixels glBindFramebuffer");
    // Read Ppxels
    glReadPixels(0, 0, this->width, this->height, this->format, this->pixelType, pixels);
    assertOpenGLError("Fbo::readToPixels glReadPixels");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    assertOpenGLError("Fbo::readToPixels glUnindFramebuffer");
}

}  // namespace OGL
