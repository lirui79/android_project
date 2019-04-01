#ifndef __SHOW_YUV_H__
#define __SHOW_YUV_H__
 
#include <pthread.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
class Renderer_yuv
{
public:
    Renderer_yuv();
    virtual ~Renderer_yuv();
 
    void requestInitEGL(EGLNativeWindowType pWindow);
    void requestRenderFrame(int surface_width, int surface_height);
    void requestDestroy();
 
    //void nativeSurfaceCreated();
 
    //void nativeSurfaceChanged(EGLint width, EGLint height);
 
    //void nativeDraw();
    unsigned char * readYUV(const char *path, int width, int height);
    void gl_initialize(void);
    void gl_uninitialize(void);
    void gl_set_framebuffer(const unsigned char* buffer, int buffersize, int width, int height);
    void gl_render_frame(int surface_width, int surface_height);
    void renderFrame(void);
    GLuint bindTexture(GLuint texture, const char *buffer, GLuint w , GLuint h);
    
    GLUtil * glutil;
private:
    enum RenderEvent
    {
        RE_NONE = 0, RE_SURFACE_CREATED, RE_SURFACE_CHANGED, RE_DRAW_FRAME, RE_EXIT
    };
 
    volatile enum RenderEvent mEnumRenderEvent;
    pthread_t mThread;
    //pthread_mutex_t mMutex;
    //pthread_cond_t mCondVar;
 
    EGLNativeWindowType mWindow;
 
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;
    
    static void *startRenderThread(void *);
 
    void initEGL();
 
    EGLint mWidth;
    EGLint mHeight;
 
    void terminateDisplay();
 
    bool mISRenderering;
 
    //int aColorLocation;
    //int aPositionLocation;
};
 
 
#endif
