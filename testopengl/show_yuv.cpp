#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
 
#include "GLUtil.h"
#include "show_yuv.h"
 
static const char* FRAG_SHADER =  
    "varying lowp vec2 tc;\n"  
    "uniform sampler2D SamplerY;\n"  
    "uniform sampler2D SamplerU;\n"  
    "uniform sampler2D SamplerV;\n"  
    "void main(void)\n"  
    "{\n"  
        "mediump vec3 yuv;\n"  
        "lowp vec3 rgb;\n"  
        "yuv.x = texture2D(SamplerY, tc).r;\n"  
        "yuv.y = texture2D(SamplerU, tc).r - 0.5;\n"  
        "yuv.z = texture2D(SamplerV, tc).r - 0.5;\n"  
        "rgb = mat3( 1,   1,   1,\n"  
                    "0,       -0.39465,  2.03211,\n"  
                    "1.13983,   -0.58060,  0) * yuv;\n"  
        "gl_FragColor = vec4(rgb, 1);\n"  
    "}\n";  
 
//Shader.vert文件内容  
static const char* VERTEX_SHADER =    
      "attribute vec4 vPosition;    \n"  
      "attribute vec2 a_texCoord;   \n"  
      "varying vec2 tc;     \n"  
      "void main()                  \n"  
      "{                            \n"  
      "   gl_Position = vPosition;  \n"  
      "   tc = a_texCoord;  \n"  
      "}                            \n";
 
Renderer_yuv::Renderer_yuv()
{
    //pthread_mutex_init(&mMutex, NULL);
    //pthread_cond_init(&mCondVar, NULL);
    mDisplay = EGL_NO_DISPLAY;
    mSurface = EGL_NO_SURFACE;
    mContext = EGL_NO_CONTEXT;
    glutil = new GLUtil();
}
 
Renderer_yuv::~Renderer_yuv()
{
    //pthread_mutex_destroy(&mMutex);
    //pthread_cond_destroy(&mCondVar);
}
 
void Renderer_yuv::requestInitEGL(EGLNativeWindowType pWindow)
{
    //pthread_mutex_lock(&mMutex);
    mWindow = pWindow;
    //mEnumRenderEvent = RE_SURFACE_CHANGED;
    initEGL();
    //nativeSurfaceCreated();
    //nativeSurfaceChanged(mWidth, mHeight);
    gl_initialize();
    
    //pthread_mutex_unlock(&mMutex);
    //pthread_cond_signal(&mCondVar);
}
void Renderer_yuv::requestRenderFrame(int surface_width, int surface_height)
{
    //pthread_mutex_lock(&mMutex);
    mEnumRenderEvent = RE_DRAW_FRAME;
    gl_render_frame(surface_width, surface_height);
    eglSwapBuffers(mDisplay, mSurface);
    //pthread_mutex_unlock(&mMutex);
    //pthread_cond_signal(&mCondVar);
}
 
void Renderer_yuv::requestDestroy()
{
    //pthread_mutex_lock(&mMutex);
    //mEnumRenderEvent = RE_EXIT;
    terminateDisplay();
    mISRenderering = false;
    //pthread_mutex_unlock(&mMutex);
    //pthread_cond_signal(&mCondVar);
}
 
void Renderer_yuv::initEGL()
{
    const EGLint attribs[] =
    { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE };
    EGLint width, height, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;
 
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
 
    eglInitialize(display, 0, 0);
 
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    surface = eglCreateWindowSurface(display, config, mWindow, NULL);
    EGLint attrs[] =
    { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, config, NULL, attrs);
 
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
    {
        printf("------EGL-FALSE\n");
        return;
    }
 
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
 
    mDisplay = display;
    mSurface = surface;
    mContext = context;
    mWidth = width;
    mHeight = height;
    printf("width:%d, height:%d\n", mWidth, mHeight);
 
}
 
void Renderer_yuv::terminateDisplay()
{
    if (mDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                EGL_NO_CONTEXT);
        if (mContext != EGL_NO_CONTEXT)
        {
            eglDestroyContext(mDisplay, mContext);
        }
        if (mSurface != EGL_NO_SURFACE)
        {
            eglDestroySurface(mDisplay, mSurface);
        }
        eglTerminate(mDisplay);
    }
 
    mDisplay = EGL_NO_DISPLAY;
    mSurface = EGL_NO_SURFACE;
    mContext = EGL_NO_CONTEXT;
 
    gl_uninitialize();
}
 
enum {  
    ATTRIB_VERTEX,  
    ATTRIB_TEXTURE,  
};  
 
static GLuint g_texYId;  
static GLuint g_texUId;  
static GLuint g_texVId;  
static GLuint simpleProgram;  
  
static unsigned char *     g_buffer = NULL;  
static int                 g_width = 0;  
static int                 g_height = 0;  
 

void Renderer_yuv::gl_initialize(void)
{  
    g_buffer = NULL;  
  
    simpleProgram = glutil->createProgram(VERTEX_SHADER, FRAG_SHADER);
    printf("##### simpleProgram: %d\n", simpleProgram);
    glUseProgram(simpleProgram);  
    glGenTextures(1, &g_texYId);  
    glGenTextures(1, &g_texUId);  
    glGenTextures(1, &g_texVId);
}  
 
void Renderer_yuv::gl_uninitialize(void)
{
    g_width = 0;  
    g_height = 0;  
  
    if (g_buffer)  
    {  
        delete[] g_buffer;  
        g_buffer = NULL;  
    }  
}  
 
void Renderer_yuv::gl_set_framebuffer(const unsigned char* buffer, int buffersize, int width, int height)  
{
    if (g_width != width || g_height != height)  
    {  
        if (g_buffer)  
           delete[] g_buffer;  
  
        g_width = width;  
        g_height = height;  
  
        g_buffer = new unsigned char[buffersize];  
    }  
 
    if (g_buffer)  
        memcpy(g_buffer, buffer, buffersize);  
  
}
 
void Renderer_yuv::gl_render_frame(int surface_width, int surface_height)  
{  
    if (0 == g_width || 0 == g_height)  
        return;  
    const char *buffer = g_buffer;  
    int width = g_width;
    int height = g_height;  

    glViewport((surface_width - width) / 2, (surface_height - height) / 2, width, height);
    bindTexture(g_texYId, buffer, width, height);  
    bindTexture(g_texUId, buffer + width * height, width/2, height/2);  
    bindTexture(g_texVId, buffer + width * height * 5 / 4, width/2, height/2);  
    renderFrame();   
}
 
void Renderer_yuv::renderFrame(void) 
{  
#if 1
    // Galaxy Nexus 4.2.2  
    static GLfloat squareVertices[] = {  
        -1.0f, -1.0f,  
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f,
    };  
  
    static GLfloat coordVertices[] = {  
        0.0f, 1.0f,  
        1.0f, 1.0f,  
        0.0f,  0.0f,  
        1.0f,  0.0f,
    };  
#else
 // HUAWEIG510-0010 4.1.1  
    static GLfloat squareVertices[] = {  
        0.0f, 0.0f,  
        1.0f, 0.0f,  
        0.0f,  1.0f,  
        1.0f,  1.0f,
    };  
  
    static GLfloat coordVertices[] = {  
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f
    };  
#endif  
  
    glClearColor(0.5f, 0.5f, 0.5f, 0.5);  
    glClear(GL_COLOR_BUFFER_BIT);
    //PRINTF("setsampler %d %d %d", g_texYId, g_texUId, g_texVId);  
    GLint tex_y = glGetUniformLocation(simpleProgram, "SamplerY");  
    GLint tex_u = glGetUniformLocation(simpleProgram, "SamplerU");  
    GLint tex_v = glGetUniformLocation(simpleProgram, "SamplerV");  
  
  
    glBindAttribLocation(simpleProgram, ATTRIB_VERTEX, "vPosition");  
    glBindAttribLocation(simpleProgram, ATTRIB_TEXTURE, "a_texCoord");  
  
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);  
    glEnableVertexAttribArray(ATTRIB_VERTEX);  
  
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, coordVertices);  
    glEnableVertexAttribArray(ATTRIB_TEXTURE);  
  
    glActiveTexture(GL_TEXTURE0);  
    glBindTexture(GL_TEXTURE_2D, g_texYId);  
    glUniform1i(tex_y, 0);  
  
    glActiveTexture(GL_TEXTURE1);  
    glBindTexture(GL_TEXTURE_2D, g_texUId);  
    glUniform1i(tex_u, 1);  
  
    glActiveTexture(GL_TEXTURE2);  
    glBindTexture(GL_TEXTURE_2D, g_texVId);  
    glUniform1i(tex_v, 2);  
  
    //glEnable(GL_TEXTURE_2D);  
    //checkGlError("glEnable");  
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  
}
 
GLuint Renderer_yuv::bindTexture(GLuint texture, const char *buffer, GLuint w , GLuint h)
{  
//  GLuint texture;
//  glGenTextures ( 1, &texture );  
    glBindTexture ( GL_TEXTURE_2D, texture );  
    glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );  
    //glBindTexture(GL_TEXTURE_2D, 0);  
 
    return texture;
}
