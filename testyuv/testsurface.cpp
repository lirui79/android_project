

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
  
#include <hardware/hwcomposer_defs.h>  
using namespace android;  

 
//将x规整为y的倍数,也就是将x按y对齐  
static int ALIGN(int x, int y) {  
    // y must be a power of 2.  
    return (x + y - 1) & ~(y - 1);  
}  
  
void render(  
        const void *data, size_t size, sp<Surface> surface,int width,int height) {  
    sp<ANativeWindow> mNativeWindow = surface;  
    int err;  
    int mCropWidth = width;  
    int mCropHeight = height;  
      
    int halFormat = HAL_PIXEL_FORMAT_YV12;//颜色空间  
    int bufWidth = (mCropWidth + 1) & ~1;//按2对齐  
    int bufHeight = (mCropHeight + 1) & ~1;  
    err = native_window_set_usage(  
            mNativeWindow.get(),  
            GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN  
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);//GRALLOC_USAGE_EXTERNAL_DISP
    if (err != 0) {
        printf("[%s][%d] native_window_set_usage %d \n",__FILE__,__LINE__, err);  
        return ;
    } 
  
    if (0 != native_window_set_scaling_mode(  
            mNativeWindow.get(),  
            NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)) {
        printf("[%s][%d] native_window_set_scaling_mode\n",__FILE__,__LINE__);  
        return ;
    }  
  
    // Width must be multiple of 32???  
    //很重要,配置宽高和和指定颜色空间yuv420  
    //如果这里不配置好，下面deque_buffer只能去申请一个默认宽高的图形缓冲区  
    if (0 != native_window_set_buffers_geometry(  
                mNativeWindow.get(),  
                bufWidth,  
                bufHeight,  
                halFormat)) {
        printf("[%s][%d] native_window_set_buffers_geometry\n",__FILE__,__LINE__);  
        return ;
    }
      
      
    ANativeWindowBuffer *buf;//描述buffer  
    //申请一块空闲的图形缓冲区  
    if ((err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(),  
            &buf)) != 0) {
        printf("[%s][%d] native_window_dequeue_buffer_and_wait %d\n",__FILE__,__LINE__ , err);   
        return;  
    }  
  
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();  
  
    Rect bounds(mCropWidth, mCropHeight);  
  
    void *dst;  
    if (0 != mapper.lock(//用来锁定一个图形缓冲区并将缓冲区映射到用户进程  
                buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst)) {
        printf("[%s][%d] mapper.lock\n",__FILE__,__LINE__);  
        return ;
    }
 
    if (true){  
        size_t dst_y_size = buf->stride * buf->height;  
        size_t dst_c_stride = ALIGN(buf->stride / 2, 16);//1行v/u的大小  
        size_t dst_c_size = dst_c_stride * buf->height / 2;//u/v的大小  
          
        memcpy(dst, data, dst_y_size + dst_c_size*2);//将yuv数据copy到图形缓冲区  
    }  
  
    if (0 != mapper.unlock(buf->handle)) {
        printf("[%s][%d] mapper.unlock\n",__FILE__,__LINE__);  
        return ;
    }
  
    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf,  
            -1)) != 0) {
        printf("[%s][%d] queueBuffer %d\n",__FILE__,__LINE__ , err);   
    }  
    buf = NULL;  
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
    
    for (int i = 0;; ++i) {
		if (getYUV12Data(fp,data,size) != 0) {//get yuv data from file;
		    printf("[%s][%d] count %d file read over\n",__FILE__,__LINE__, i);
		    break;
		}
	  
		SurfaceComposerClient::openGlobalTransaction();  
		surfaceControl->setSize(width, height);  
		surfaceControl->setPosition(0, 0);  
		SurfaceComposerClient::closeGlobalTransaction();  
		surfaceControl->show();//感觉没有这步，图片也能显示  

		
		render(data,size,surface,width,height);  
		printf("[%s][%d] count %d\n",__FILE__,__LINE__, i);
		usleep(16000);
    }
    
    fclose(fp);
    delete[]  data;


/*
    SurfaceComposerClient::openGlobalTransaction();  
    surfaceControl->setSize(480, 272);  
    surfaceControl->setPosition(100, 100);  
    SurfaceComposerClient::closeGlobalTransaction();  
    surfaceControl->show();//感觉没有这步，图片也能显示  
      
    FILE *fp = fopen("/tmp/rgb565.rgb","rb");//我们从一个文件里获取RGB565图像数据 480*272  
    if(fp != NULL){  
        unsigned char *rgb565Data = new unsigned char[480*272*2];  
        memset(rgb565Data,0x00,480*272*2);  
        fread(rgb565Data,1,480*272*2,fp);  
        surface->lock(&outBuffer,NULL);  
        //bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);  
        //android_memset16((uint16_t*)outBuffer.bits, 0x04E0, bpr*outBuffer.height);  
        memcpy(outBuffer.bits,rgb565Data,480*272*2);  
        delete[] rgb565Data;  
        surface->unlockAndPost();  
    }  
    fclose(fp);  
    sleep(3);  
*/

//用skia画图  
/*******************************第三张图***************************************************/  
/*
    SurfaceComposerClient::openGlobalTransaction();  
    surfaceControl->setSize(320, 420);  
    surfaceControl->setPosition(100, 100);  
    SurfaceComposerClient::closeGlobalTransaction();  
    surfaceControl->show();//感觉没有这部图片也能显示  
      
    SkPaint paint;  
    paint.setColor(SK_ColorBLUE);  
    Rect rect(0, 0, 320, 240);  
    Region dirtyRegion(rect);  
      
    surface->lock(&outBuffer, &rect);  
    bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);  
//    printf("w=%d,h=%d,bpr=%d,fmt=%d,bits=%p\n", outBuffer.w, outBuffer.h, bpr, outBuffer.format, outBuffer.bits);  
    SkBitmap bitmap;  
    bitmap.setConfig(convertPixelFormat(outBuffer.format), 320, 240, bpr);  
    bitmap.setPixels(outBuffer.bits);  
    SkCanvas canvas;  
    SkRegion clipReg;  
    const Rect b(dirtyRegion.getBounds());  
    clipReg.setRect(b.left, b.top, b.right, b.bottom);  
    canvas.clipRegion(clipReg);  
    canvas.drawARGB(0, 0xFF, 0x00, 0xFF);  
    canvas.drawCircle(200, 200, 100, paint);  
    bitmap.notifyPixelsChanged();  
    surface->unlockAndPost();  
    sleep(3);  
   */
  
/**********************************************************************************/  
 /*   SkFILEStream stream("/sdcard/test.jpg");  
    SkImageDecoder* codec = SkImageDecoder::Factory(&stream);  
    if(codec){  
        SkBitmap bmp;  
        stream.rewind();  
        codec->decode(&stream, &bmp, SkBitmap::kRGB_565_Config, SkImageDecoder::kDecodePixels_Mode);  
        surface->lock(&outBuffer,NULL);  
        bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);  
        bitmap.setConfig(convertPixelFormat(outBuffer.format), 320, 240, bpr);  
        bitmap.setPixels(outBuffer.bits);  
        //dev = new SkDevice(bitmap);  
        //canvas.setDevice(dev);  
        canvas.drawBitmap(bmp, SkIntToScalar(200), SkIntToScalar(300));  
        surface->unlockAndPost();  
        sleep(3);  
        //delete dev;  
    }  */
      
    IPCThreadState::self()->joinThreadPool();  
      
    IPCThreadState::self()->stopProcess();  
  
    return 0;  
}
