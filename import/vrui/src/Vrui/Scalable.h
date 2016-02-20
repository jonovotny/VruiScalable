#ifdef WIN32
#  include <windows.h>
#endif


#ifdef WIN32
#include <GL/gl.h>
#include "glext.h"
#else
//#include <GL/gl.h>
//#include <GL/glx.h>
#endif

#include <string>
#include <set>


//void ScalableSetView(double, double, double, bool left=false);
void ScalableSetView0(double, double, double, bool left=false);

void ScalableInit(const char* ScalableMesh);
void ScalableClose();
void ScalablePreSwap(bool left=false);

void getTopLeft(double &x, double &y, double &z, bool left);
void getTopRight(double &x, double &y, double &z, bool left);
void getBotLeft(double &x, double &y, double &z, bool left);
void getBotRight(double &x, double &y, double &z, bool left);

void ScalableSetEye(bool left=false);
extern bool useScalable;

/*struct GLExtensions
{
public:
  
  // Description:
  // Get the full list of extensions we have.
  void Initialize();
  
  // Description:
  // Is a particular extension supported.
  inline bool IsExtensionSupported(const char *ExtensionText)
  {
    return (m_glExtSet.find(ExtensionText) != m_glExtSet.end());
  }
  
  // Description:
  // Get the extension function, return NULL if not ssupported.
  //static void * GetFunction(const char *ExtensionText);
  
  // Description:
  // Once we grab during the initialize. They could be NULL.
  PFNGLGENFRAMEBUFFERSEXTPROC            glGenFramebuffersEXT;
  PFNGLDELETEFRAMEBUFFERSEXTPROC         glDeleteFramebuffersEXT;
  PFNGLBINDFRAMEBUFFEREXTPROC            glBindFramebufferEXT;
  PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC     glCheckFramebufferStatusEXT;
  PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC 
                                         glGetFramebufferAttachmentParameterivEXT;
  PFNGLGENERATEMIPMAPEXTPROC             glGenerateMipmapEXT;
  PFNGLFRAMEBUFFERTEXTURE2DEXTPROC       glFramebufferTexture2DEXT;
  PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC    glFramebufferRenderbufferEXT;
  PFNGLGENRENDERBUFFERSEXTPROC           glGenRenderbuffersEXT;
  PFNGLDELETERENDERBUFFERSEXTPROC        glDeleteRenderbuffersEXT;
  PFNGLBINDRENDERBUFFEREXTPROC           glBindRenderbufferEXT;
  PFNGLRENDERBUFFERSTORAGEEXTPROC        glRenderbufferStorageEXT;
  PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
  PFNGLISRENDERBUFFEREXTPROC             glIsRenderbufferEXT;
  PFNGLACTIVETEXTUREARBPROC              glActiveTexture;

protected:
  std::set<std::string> m_glExtSet;
  
  // Description:
  // Fill in the set of strings.
  static void GetOpenGLExtensions(std::set<std::string>& glExtSet);
};*/

