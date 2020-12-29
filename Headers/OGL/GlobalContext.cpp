#include "GlobalContext.h"
#include "Debug.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace OGL {

GlobalContext::GlobalContext() {
    
    angle::Library *mEntryPointsLib = angle::OpenSharedLibrary("libEGL", angle::SearchType::ApplicationDir);
    
    PFNEGLGETPROCADDRESSPROC getProcAddress;
    mEntryPointsLib->getAs("eglGetProcAddress", &getProcAddress);
    if (!getProcAddress) {
        return;
    }
    
    angle::LoadEGL(getProcAddress);

    
    if (!eglGetPlatformDisplayEXT) {
        return;
    }
    
    EGLint dispattrs[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
        EGL_NONE
    };

    display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                         reinterpret_cast<void *>(EGL_DEFAULT_DISPLAY),
                                         dispattrs);
    
    eglInitialize(display, nullptr, nullptr);
    assertEGLError("eglInitialize");
    
    
    EGLConfig config;
    EGLint num_config;
    
    eglChooseConfig(display, nullptr, &config, 1, &num_config);
    if (!assertEGLError("eglChooseConfig")) {
        return;
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    if (!assertEGLError("eglBindAPI")) {
        return;
    }

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    if (!assertEGLError("eglCreateContext")) {
        return;
    }
    
    surface = eglCreatePbufferSurface(display, config, nullptr);
    if (!assertEGLError("eglCreatePbufferSurface")) {
        return;
    }

    // On succeeded
    this->initialized = true;
}

void GlobalContext::bind() {
    eglMakeCurrent(this->display, this->surface, this->surface, this->context);
}

bool GlobalContext::assertEGLError(const std::string &msg) {
    GLenum error = glGetError();

    if (error != GL_NO_ERROR) {
        FX_LOG("OpenGL error 0x" << std::hex << error << " at " << msg);
        return false;
    } else {
        return true;
    }
}

GlobalContext::~GlobalContext() {
    eglDestroyContext(this->display, this->context);
    eglTerminate(this->display);
    
}

}  // namespace OGL
