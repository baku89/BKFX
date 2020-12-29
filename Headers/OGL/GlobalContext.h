#include <sstream>

#include "common/system_utils.h"
#include "util/egl_loader_autogen.h"

namespace OGL {

class GlobalContext {
   public:
    bool initialized = false;
    GlobalContext();
    ~GlobalContext();
    void bind();

   private:
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    
    bool assertEGLError(const std::string& msg);
};

}  // namespace OGL
