#include "SurfaceSource.h"

#include <utils/Log.h>

SurfaceSource::SurfaceSource() {        
}

SurfaceSource::~SurfaceSource() {
}

int SurfaceSource::onCreate(int w , int h) {
    width  = w;
    height = h;
    BufferQueue::createBufferQueue(&inputProducer, &inputConsumer);
    inputConsumer->setDefaultBufferSize(width, height);
    status_t err = StreamSplitter::createSplitter(inputConsumer, &splitter);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to StreamSplitter::createSplitter (err=%d)\n", err);
        return -1;
    }   
    IGraphicBufferProducer::QueueBufferOutput qbo;
    inputProducer->setAsyncMode(true);
    err = inputProducer->connect(new DummyProducerListener, NATIVE_WINDOW_API_CPU, false,
            &qbo);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to IGraphicBufferProducer::connect (err=%d)\n", err);
        return -1;
    }   
    return 0;
}


int SurfaceSource::getYUV12Data(FILE *fp, unsigned char * pYUVData,int size, int nY4M) {
    int sz = fread(pYUVData,size,1,fp);
    if (nY4M != 0) {
         unsigned char temp[6] = "12345" ;
         fread(temp,6,1,fp);
    }
    if (sz > 0)
       return 0;
    return -1;
}

void* SurfaceSource::getNativeWindow() {
    if (surface == NULL) {
        surface = new Surface(inputProducer, false);
        window = surface;
    }
    return window.get();
}


void SurfaceSource::render(const void *data, size_t size,int width,int height) {
    int slot = -1;
    sp<Fence> fence;
    sp<GraphicBuffer> buffer;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_VIDEO_ENCODER; //| GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    inputProducer->dequeueBuffer(&slot, &fence, width, height, HAL_PIXEL_FORMAT_YV12, usage);//HAL_PIXEL_FORMAT_YV12

    inputProducer->requestBuffer(slot, &buffer);

    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
    Rect rect(0, 0, width, height);// HAL_DATASPACE_ARBITRARY  HAL_DATASPACE_UNKNOWN HAL_DATASPACE_V0_BT709 HAL_DATASPACE_V0_JFIF
    IGraphicBufferProducer::QueueBufferInput input = IGraphicBufferProducer::QueueBufferInput(nowTime, false, HAL_DATASPACE_UNKNOWN, rect,
              NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, Fence::NO_FENCE);
    IGraphicBufferProducer::QueueBufferOutput output;
   
    void *dataIn = NULL ;
    buffer->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, reinterpret_cast<void**>(&dataIn));
    memcpy(dataIn, data, size);//将yuv数据copy到图形缓冲区  
    buffer->unlock();
    inputProducer->queueBuffer(slot, input, &output);
}

void SurfaceSource::setdata(const void *data, size_t size,int width,int height) {
    int slot = -1;
    sp<Fence> fence;
    sp<GraphicBuffer> buffer;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;  
            //| GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    inputProducer->dequeueBuffer(&slot, &fence, width, height, HAL_PIXEL_FORMAT_RGBA_8888, usage);

    inputProducer->requestBuffer(slot, &buffer);

    int64_t nowTime = systemTime(CLOCK_MONOTONIC);
    Rect rect(0, 0, width, height);// HAL_DATASPACE_ARBITRARY  HAL_DATASPACE_UNKNOWN
    IGraphicBufferProducer::QueueBufferInput input = IGraphicBufferProducer::QueueBufferInput(nowTime, false, HAL_DATASPACE_UNKNOWN, rect,
              NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, Fence::NO_FENCE);
    IGraphicBufferProducer::QueueBufferOutput output;
   
    void *dataIn = NULL ;
    buffer->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, reinterpret_cast<void**>(&dataIn));
    memcpy(dataIn, data, size);//将RGBA copy到图形缓冲区  
    buffer->unlock();
    inputProducer->queueBuffer(slot, input, &output);
}  

int SurfaceSource::setRGBAData(int width, int height) {
    int size = width * height ;
    unsigned char *data = new unsigned char[4 * size];  
    int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000, curTime, diff;
    for (int i = 0 ; i < 500 ; ++i) {
        memset(data , 0 , 4 * size);
        for (int j = 0 ; j < size; ++j) 
           data[4 * j + (i%4)] = 255;
        setdata(data, 4 * size, width, height);
        curTime = systemTime(CLOCK_MONOTONIC) / 1000;
        diff = curTime - nowTime;
        ALOGI("timestamp %zu %zu render\n", curTime / 1000, diff / 1000);
        if (diff < 16000)
           usleep(16000 - diff);
        nowTime = curTime;
    }
    
    delete[]  data;
    return 0;
}

int SurfaceSource::setData(const char *fileName, int width, int height) {
    if (fileName == NULL) {
        return setRGBAData(width, height);
    }

    int size = width * height * 3/2;  
    unsigned char *data = new unsigned char[size];  
    FILE *fp = fopen(fileName,"rb");
    ALOGI("fopen %s\n", fileName);
    if(fp == NULL){  
        ALOGE("read %s fail !!!!!!!!!!!!!!!!!!!\n",fileName);
        delete []data;
        return -1;  
    }

    int nY4M = 0;
    if (strstr(fileName, ".y4m") != NULL) {
        ALOGI("%s is *.y4m\n", fileName);
        nY4M = 1;
        char buffer[6] = "FRAME";
        buffer[5] = '\0';
        char temp[10] = "\0" ;
        
        while(memcmp(buffer, temp, 5) != 0) { 
            if (fscanf(fp, "%s ", temp) <= 0)
               break;
            
            ALOGI("source %s dest %s\n", buffer, temp);   
        }
    }
    
    int64_t nowTime = systemTime(CLOCK_MONOTONIC) / 1000, curTime, diff;
    for (int i = 0;; ++i) {
		if (getYUV12Data(fp, data, size, nY4M) != 0) {//get yuv data from file;
		    ALOGE("[%s][%d] count %d file read over\n",__FILE__,__LINE__, i);
		    break;
		}
        render(data, size, width, height);
        curTime = systemTime(CLOCK_MONOTONIC) / 1000;
        diff = curTime - nowTime;
        ALOGI("timestamp %zu %zu render\n", curTime / 1000, diff / 1000);
        if (diff < 16000)
           usleep(16000 - diff);
        nowTime = curTime;
    }
    
    fclose(fp);
    delete[]  data;

    return 0;
}

int SurfaceSource::onClose() {

    return 0;
}

int SurfaceSource::addOutput(sp<IGraphicBufferProducer> outputProducer) {
    status_t err = splitter->addOutput(outputProducer);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to addOutput (err=%d)\n", err);
        return -2;
    }

    return 0;
}