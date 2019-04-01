

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
#include <ui/Rect.h>  
#include <ui/Region.h>  
#include <android/native_window.h>  
#include <gui/IGraphicBufferProducer.h>

#include <ui/GraphicBufferMapper.h>  
  
#include <hardware/hwcomposer_defs.h>  
using namespace android;  

  
void render(const void *data, size_t size, sp<IGraphicBufferProducer> inputProducer, int width, int height) {  
    int slot = -1;
    sp<Fence> fence;
    sp<GraphicBuffer> buffer;
    int usage = GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN  
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    inputProducer->dequeueBuffer(&slot, &fence, width, height, HAL_PIXEL_FORMAT_YV12, usage);

    inputProducer->requestBuffer(slot, &buffer);

    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
    Rect rect(0, 0, width, height);
    IGraphicBufferProducer::QueueBufferInput input = IGraphicBufferProducer::QueueBufferInput(nowTime, false, HAL_DATASPACE_UNKNOWN, rect,
              NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, fence);
    IGraphicBufferProducer::QueueBufferOutput output;
   
    void *dataIn = NULL ;
    buffer->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, reinterpret_cast<void**>(&dataIn));
    memcpy(dataIn, data, size);//将yuv数据copy到图形缓冲区  
    buffer->unlock();
    inputProducer->queueBuffer(slot, input, &output);
}  
  
bool getYV12Data(const char *path,unsigned char * pYUVData,int size){  
    FILE *fp = fopen(path,"rb");  
    if(fp == NULL){  
        printf("read %s fail !!!!!!!!!!!!!!!!!!!\n",path);  
        return false;  
    }  
    fread(pYUVData,size,1,fp);  
    fclose(fp);  
    return true;  
}

int getYUV12Data(FILE *fp, unsigned char * pYUVData,int size) {
    int sz = fread(pYUVData,size,1,fp);
    if (sz > 0)
       return 0;
    return -1;
}



// testBufferProducer 352 288 /sdcard/akiyo_cif.yuv
int main(int argc, char** argv)  
{  
    if (argc < 3) {
    
      printf("please input w h and yuv file\n");
      
      return -1;
    }
    
    printf("input w %s h %s yuv file %s\n" , argv[1] , argv[2] , argv[3]);
    // set up the thread-pool  
    sp<ProcessState> proc(ProcessState::self());  
    ProcessState::self()->startThreadPool();  
  
    // create a client to surfaceflinger  
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();  
    //DisplayoutBuffer display;  
    //client->getDisplayoutBuffer(client->getBuiltInDisplay(HWC_DISPLAY_PRIMARY), &display);  
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdMain));  
    DisplayInfo dinfo;  
    //获取屏幕的宽高等信息  
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);  
    printf("w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f\n",   
        dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density);  
    if (status)  
        return -1;  
    sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),  
            dinfo.w, dinfo.h, PIXEL_FORMAT_RGBA_8888, 0);  
  
/****************************第一张图******************************************************/  
    SurfaceComposerClient::openGlobalTransaction();  
    surfaceControl->setLayer(100000);//设定Z坐标  
    surfaceControl->setPosition(0, 0);//以左上角为(0,0)设定显示位置  
    SurfaceComposerClient::closeGlobalTransaction();  
    surfaceControl->show();//感觉没有这步,图片也能显示  
    sp<Surface> surface = surfaceControl->getSurface();
    sp<IGraphicBufferProducer> inputProducer = surface->getIGraphicBufferProducer();
  
    ANativeWindow_Buffer outBuffer;  
    //Surface::SurfaceoutBuffer outBuffer;  
    surface->lock(&outBuffer,NULL);//获取surface缓冲区的地址  
    ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);  
    android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);//往surface缓冲区塞要显示的RGB内容  
    surface->unlockAndPost();  
    sleep(1);  
 
//从文件中获取RGB数据显示 下载地址：http://kc.cc/WeVp  
/****************************第二张图******************************************************/
      
//    int width = 352,height = 288;       
    int width = atoi(argv[1]) ,height = atoi(argv[2]); 
    int size = width * height * 3/2;  
    unsigned char *data = new unsigned char[size];  
    const char *path = argv[3];//"/sdcard/akiyo_cif.yuv";
    FILE *fp = fopen(path,"rb");  
    if(fp == NULL){  
        printf("read %s fail !!!!!!!!!!!!!!!!!!!\n",path);
        delete []data;
        return -1;  
    }
    

    SurfaceComposerClient::openGlobalTransaction();  
    surfaceControl->setSize(width, height);  
    surfaceControl->setPosition(100, 100);  
    SurfaceComposerClient::closeGlobalTransaction(); 
    surfaceControl->show();

    for (int i = 0;; ++i) {
		if (getYUV12Data(fp,data,size) != 0) {//get yuv data from file;
		    printf("[%s][%d] count %d file read over\n",__FILE__,__LINE__, i);
		    break;
		}
	  

		//感觉没有这步，图片也能显示  		
		render(data,size,inputProducer,width,height);  
		printf("[%s][%d] count %d\n",__FILE__,__LINE__, i);
		usleep(16000);
    }
    
    fclose(fp);
    delete[]  data;
      
    IPCThreadState::self()->joinThreadPool();  
      
    IPCThreadState::self()->stopProcess();  
  
    return 0;  
}
