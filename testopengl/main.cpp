/*main.cpp*/
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
 
#include "GLUtil.h"
//#include "Renderer.h"
#include "show_yuv.h"
 
using namespace android;

int getYV12Data(FILE * fp, unsigned char * pYUVData, int size, int offset)
{
    int ret = -1;
    //FILE *fp = fopen(path,"rb");
    if(fp == NULL)
    {
        printf("fp == NULL!\n");
        return -1;
    }
    
    //ret = fseek(fp, size * offset, SEEK_SET);
    printf("### offset: %d\n", offset);
    ret = fseek(fp, size, SEEK_CUR);
    if(ret != 0)
    {
        return -1;
    }
    
    ret = fread(pYUVData, size, 1, fp);
    if(ret == 0)
    {
        return -1;
    }
    //fclose(fp);
 
    return 0;
}

int getYUV12Data(FILE *fp, unsigned char * pYUVData,int size) {
    int sz = fread(pYUVData,size,1,fp);
    if (sz > 0)
       return 0;
    return -1;
}

 
int main(int argc, char** argv)
{
    if (argc < 3) {
    
      printf("please input w h and yuv file\n");
      
      return -1;
    }
    
    printf("input w %s h %s yuv file %s\n" , argv[1] , argv[2] , argv[3]);
    int i = 0;
    int ret = 0;
    FILE * fp = NULL;
    unsigned char * buffer = NULL;
    Renderer_yuv * mRenderer = NULL;
    mRenderer = new Renderer_yuv();
    int win_width  = atoi(argv[1]);
    int win_height = atoi(argv[2]);
    //mRenderer->start();
    EGLNativeWindowType WindowTypes = (EGLNativeWindowType) mRenderer->glutil->getNativeWindow(win_width, win_height);
    mRenderer->requestInitEGL(WindowTypes);
    printf("request init egl\n");
     const char *path = argv[3];
    fp = fopen(path, "rb");
    int buffer_size = win_width * win_height * 3 / 2;
    buffer = new unsigned char[buffer_size];
 
    for(i = 1; ; i++)
    {
        printf("get frame %d\n", i);
        ret = getYUV12Data(fp, buffer, buffer_size);
        if(ret < 0) {
            printf("play end!\n");
            break;
        }
        printf("set render frame\n");
        mRenderer->gl_set_framebuffer(buffer, buffer_size, win_width, win_height);
        printf("render frame\n");
        mRenderer->requestRenderFrame(win_width, win_height);
        usleep(16000);
    }
 
    fclose(fp);
    printf("render over\n");
    mRenderer->requestDestroy();
    delete mRenderer;
    
    return 0;
}

