#include <include/SoftwareRenderer.h>
 
#include <cutils/memory.h>
 
#include <unistd.h>
#include <utils/Log.h>
 
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
 
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>
#include <android/native_window.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/foundation/AMessage.h>
 
 
#include <ui/GraphicBuffer.h>
#include <gui/Surface.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
 
#ifndef NATIVEGLESVIEW_GLUTIL_H
#define NATIVEGLESVIEW_GLUTIL_H
 
#include <GLES2/gl2.h>
#include <android/log.h>
//#define LOGI(level, ...) __android_log_print(ANDROID_LOG_INFO, "NATIVE_LOG", __VA_ARGS__)
#define LOGI(level, format, ...) printf("[%s: %d]" format, __FILE__, __LINE__, ##__VA_ARGS__)
 
using namespace android;
 
class GLUtil
{
 
public:
    int compileShader(int type, const char* shaderCode);
    int createProgram(const char * vertexShaderCode, const char * fragmentShaderCode);
 
    EGLNativeWindowType getNativeWindow(int width, int hight);
    void disposeNativeWindow(void);
    
private:
    sp<SurfaceComposerClient>  mComposerClient;
    sp<SurfaceControl>   mSurfaceControl;
};
 
#endif 
