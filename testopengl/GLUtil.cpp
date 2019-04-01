#include "GLUtil.h"
 
int GLUtil::compileShader(int type, const char * shaderCode)
{
 
    int shader = glCreateShader(type);
    if (shader == 0)
    {
        printf("Create Sharder error %d", glGetError());
        return 0;
    }
 
    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);
    GLint compileStatus = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus)
    {
        glDeleteShader(shader);
        LOGI(1, "compile shader error");
        return 0;
    }
 
    return shader;
}
 
int GLUtil::createProgram(const char *vertexShaderCode, const char *fragmentShaderCode)
{
    GLint program = glCreateProgram();
    if (0 == program)
    {
        LOGI(1, "create program error");
        return 0;
    }
 
    LOGI(1, "create program success");
    int vertexShaderID = compileShader(GL_VERTEX_SHADER, vertexShaderCode);
    int fragmentShaderID = compileShader(GL_FRAGMENT_SHADER,
            fragmentShaderCode);
 
    glAttachShader(program, vertexShaderID);
    glAttachShader(program, fragmentShaderID);
    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (0 == linkStatus)
    {
        glDeleteProgram(program);
        LOGI(1, "link program error");
        return 0;
    }
 
    return program;
}
 
EGLNativeWindowType GLUtil::getNativeWindow(int width, int hight)
{
    DisplayInfo dinfo;
    
    mComposerClient = new SurfaceComposerClient();
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
 
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    printf("w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f\n", 
                dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density);
 
    mSurfaceControl = mComposerClient->createSurface(
        String8("Test Surface"),
        dinfo.w, dinfo.h,
        PIXEL_FORMAT_RGBA_8888, 0);
 
    SurfaceComposerClient::openGlobalTransaction();
    mSurfaceControl->setLayer(INT_MAX);//设定Z坐标
    mSurfaceControl->setPosition((dinfo.w - width) / 2, (dinfo.h - hight) / 2);
    mSurfaceControl->setSize(width, hight);
 
    SurfaceComposerClient::closeGlobalTransaction();
 
    sp<ANativeWindow> window = mSurfaceControl->getSurface();
 
    return window.get();
}
 
 
void GLUtil::disposeNativeWindow(void)
{
    if (mComposerClient != NULL) 
    {
        mComposerClient->dispose();
        mComposerClient = NULL;
        mSurfaceControl = NULL;
    }
}
