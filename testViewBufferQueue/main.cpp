#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "IMediaEncode.h"
#include <binder/IPCThreadState.h>  
#include <binder/ProcessState.h>



#include <dlfcn.h>

using namespace android;


static void *GetFunction(const char *dllName, const char *functionName){
	const char *error;
	if ((error = dlerror()) != NULL) {
		ALOGI("Before GetNewClass [%s]-[%s] reports error %s", dllName , functionName , error);
	}
	void *lib_handle = dlopen(dllName, RTLD_LAZY);
	if (!lib_handle) {
		ALOGI("dlopen [%s]-[%s] reports error %s", dllName , functionName , dlerror());
		return NULL;
	}
	void* fn = dlsym(lib_handle, functionName);
	if (fn == NULL) {
	    if ((error = dlerror()) != NULL)  {
	    	ALOGI("dlsym [%s]-[%s] reports error %s", dllName , functionName , error);
	    } else {
	    	ALOGI("dlsym [%s]-[%s] reports error return NULL", dllName , functionName);
	    }
		return NULL;
	}

	if ((error = dlerror()) != NULL) {
		ALOGI("dlsym [%s]-[%s] reports error %s", dllName , functionName , error);
	}
	return fn;
}

//testVB 352 288 /sdcard/akiyo_cif.yuv
int main(int argc, char** argv)  
{   
    
    if (argc < 3) {
   
      printf("please input w h and yuv file\n");
      
      return -1;
    }
    
    printf("input w %s h %s" , argv[1] , argv[2]);
    int width  = atoi(argv[1]);
    int height = atoi(argv[2]);
    const char *yuvfile = NULL;
    int yuv = 0;
    if (argc == 4) {
        yuvfile = argv[3] ;
        yuv = 1;
        printf(" yuv file %s" , argv[3]);
    }
    fprintf(stderr, "\n");
    getIMediaEncodeType getIMediaEncodeFn = (getIMediaEncodeType) GetFunction("libMediaEncode.so", "getIMediaEncode");

    IMediaEncode *mediaEncode = getIMediaEncodeFn();
    mediaEncode->onCreate(mediaEncode, "media surface", width, height, yuv);
    mediaEncode->onEncode(mediaEncode);
    mediaEncode->onSetData(mediaEncode, yuvfile, width, height);
 
 /*
    SurfaceSource surfaceSource;
    SurfaceView   surfaceView[3] ;
    SurfaceEncode surfaceEncode;

    sp<ProcessState> proc(ProcessState::self());  
    ProcessState::self()->startThreadPool(); 

    surfaceSource.onCreate(width, height);
    surfaceView[0].createSurface("testViewBufferQueue", width, height, 0, 0);
    surfaceSource.addOutput(surfaceView[0].getIGraphicBufferProducer());
   // surfaceView[1].createSurface("testViewBufferQueue", width, height, 100, 300);
   // surfaceSource.addOutput(surfaceView[1].getIGraphicBufferProducer());


   // surfaceView[2].createSurface("testViewBufferQueue", width, height, 100, 600);
   // surfaceSource.addOutput(surfaceView[2].getIGraphicBufferProducer());
    surfaceEncode.onCreate(width, height, yuv);
    printf("encode create\n");
    surfaceSource.addOutput(surfaceEncode.getIGraphicBufferProducer());
    printf("encode start\n");
    surfaceEncode.start();
    printf("setdata\n");
    surfaceSource.setData(yuvfile, width, height);//*/

    IPCThreadState::self()->joinThreadPool();      
    IPCThreadState::self()->stopProcess();  
    return 0;
}