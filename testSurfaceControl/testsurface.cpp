

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

#include <ui/GraphicBufferMapper.h>  
  
#include <gui/DisplayEventReceiver.h>
#include <hardware/hwcomposer_defs.h>  
using namespace android; 

typedef struct BufferInfo {
    uint8_t    *data;
    uint32_t    size;
    uint32_t    width;
    uint32_t    height;
    PixelFormat format;
    uint32_t    stride;
    Rect        crop;
    uint32_t    transform;
    uint32_t    scalingMode;
    int64_t     timestamp;
    android_dataspace dataSpace;
    uint64_t    frameNumber;
} BufferInfo;
 
//将x规整为y的倍数,也就是将x按y对齐  
static int ALIGN(int x, int y) {  
    // y must be a power of 2.  
    return (x + y - 1) & ~(y - 1);  
} 
  
void setdata(sp<IGraphicBufferProducer> inputgbp, struct BufferInfo *buf) {
    int slot = -1;
    sp<Fence> fence;
    sp<GraphicBuffer> buffer;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN; 
#ifdef PLATFORM_VERSION_7
    inputgbp->dequeueBuffer(&slot, &fence, buf->width, buf->height, buf->format, usage);
#endif

#ifdef PLATFORM_VERSION_8
    inputgbp->dequeueBuffer(&slot, &fence, buf->width, buf->height, buf->format, usage, NULL, NULL);
#endif

    inputgbp->requestBuffer(slot, &buffer);

    Rect rect(0, 0, buf->width, buf->height);// HAL_DATASPACE_ARBITRARY  HAL_DATASPACE_UNKNOWN
    IGraphicBufferProducer::QueueBufferInput input = IGraphicBufferProducer::QueueBufferInput(buf->timestamp, false, buf->dataSpace, rect,
              NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, fence);
    IGraphicBufferProducer::QueueBufferOutput output;
   
    void *dataIn = NULL ;
    buffer->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, &dataIn);
    memcpy(dataIn, buf->data, buf->size);//将RGBA copy到图形缓冲区  
    buffer->unlock();
    inputgbp->queueBuffer(slot, input, &output);
    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
 //   printf("SurfaceSource::setdata %lld  %lld %lld", nowTime, buf->timestamp, (nowTime - buf->timestamp)/1000000);
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



//testsurface 352 288 /sdcard/akiyo_cif.yuv
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

    int layerStack = 0; 
    //DisplayoutBuffer display;  
    //client->getDisplayoutBuffer(client->getBuiltInDisplay(HWC_DISPLAY_PRIMARY), &display);
#ifdef PLATFORM_VERSION_7
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdMain));
#endif

#ifdef PLATFORM_VERSION_8
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdHdmi));
    
    layerStack = 10; 
#endif

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

	SurfaceComposerClient::setDisplayLayerStack(dtoken, layerStack); 
    SurfaceComposerClient::openGlobalTransaction();  
	surfaceControl->setLayerStack(layerStack); 
    surfaceControl->setLayer(300000);//设定Z坐标  
    surfaceControl->setPosition(0, 0);//以左上角为(0,0)设定显示位置  
    surfaceControl->show();//感觉没有这步,图片也能显示  
    SurfaceComposerClient::closeGlobalTransaction(); 


    sp<Surface> surface = surfaceControl->getSurface();  
  
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

    DisplayEventReceiver mReceiver;
    if (mReceiver.initCheck() != NO_ERROR) {
        printf("DisplayEventReceiver error\n");
        return -3;
    }
    mReceiver.setVsyncRate(1);

    printf("beging yuv data\n");
    struct BufferInfo bufferInfo;
    bufferInfo.size = 4 * width * height ;
    bufferInfo.stride = width;
    bufferInfo.width = width;
    bufferInfo.height = height;
    bufferInfo.format = HAL_PIXEL_FORMAT_YV12;
    bufferInfo.timestamp = systemTime(CLOCK_MONOTONIC);
    if (bufferInfo.format == HAL_PIXEL_FORMAT_YV12) {//HAL_PIXEL_FORMAT_YCbCr_420_888 HAL_PIXEL_FORMAT_YV12
        //bufferInfo.size = bufferInfo.stride * bufferInfo.height * 3/2;
       bufferInfo.size = bufferInfo.stride * bufferInfo.height * 3/2;
    } else {
        size = width * height * 3/2;
       // yuv = 0;
    }

    bufferInfo.dataSpace = HAL_DATASPACE_UNKNOWN;

	SurfaceComposerClient::setDisplayLayerStack(dtoken, layerStack); 
    SurfaceComposerClient::openGlobalTransaction();  
	surfaceControl->setLayerStack(layerStack); 
    surfaceControl->setSize(width, height);  
    surfaceControl->setPosition(0, 0);  
    surfaceControl->show();//感觉没有这步，图片也能显示  
    SurfaceComposerClient::closeGlobalTransaction();
    
    surface = surfaceControl->getSurface();  
    sp<IGraphicBufferProducer> gbp = surface->getIGraphicBufferProducer();
    DisplayEventReceiver::Event  event ;
    //int fd = mReceiver->getFd() ;
    size_t sz = 0, count = 1;
    fd_set fds;
    struct timeval timeout={1,0};
    int sockid = mReceiver.getFd();

    int64_t timestart = 0, curtime = 0;
    double startcount = 0, endcount = 0;
    double fps = 0;
    for (int i = 0;; ++i) {
        FD_ZERO(&fds); //每次循环都要清空集合，否则不能检测描述符变化
        FD_SET(sockid, &fds); //添加描述符
        int code = select(sockid + 1, &fds, NULL, NULL, &timeout);
        if (code <= 0) {
            continue;
        }
        sz = mReceiver.getEvents(&event, count);
        if (sz <= 0) {
            continue;
        }

        if (timestart == 0) {
            timestart = event.header.timestamp;
            startcount = event.vsync.count;
        } else {
            curtime =  event.header.timestamp;
            endcount = event.vsync.count;
        }

        if ((event.vsync.count % 100) == 0) {
            fps = 1000000000 * (endcount - startcount) / (curtime - timestart);
            timestart = 0;
            printf("fps = %f \n", fps);
        }


        printf("event[%x %x %lX %x]\n", event.header.type, event.header.id, event.header.timestamp, event.vsync.count);

		if (getYUV12Data(fp,data,size) != 0) {//get yuv data from file;
		    printf("[%s][%d] count %d file read over\n",__FILE__,__LINE__, i);
		    break;
		}
	  
        bufferInfo.data = data;

//感觉没有这步，图片也能显示  

		setdata(gbp, &bufferInfo);
		//render(data,size,surface,width,height);  
		printf("[%s][%d] count %d\n",__FILE__,__LINE__, i);
		usleep(16000);
    }
    
    fclose(fp);
    delete[]  data;

      
    IPCThreadState::self()->joinThreadPool();  
      
    IPCThreadState::self()->stopProcess();  
  
    return 0;  
}
