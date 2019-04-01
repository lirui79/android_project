#ifndef SURFACE_SOURCE_H
#define SURFACE_SOURCE_H

#include <gui/BufferItem.h>
#include <gui/BufferQueue.h>
#include <gui/Surface.h>
#include <gui/IConsumerListener.h>
#include <gui/ISurfaceComposer.h>
#include <gui/StreamSplitter.h>
#include <private/gui/ComposerService.h>

using namespace android;

class SurfaceSource {
public:
    SurfaceSource();
    virtual ~SurfaceSource();

    int onCreate(int w , int h) ;

    int setData(const char *fileName, int width, int height) ;

    int onClose();

    int addOutput(sp<IGraphicBufferProducer> outputProducer) ;

    void* getNativeWindow();

protected:
    int  getYUV12Data(FILE *fp, unsigned char * pYUVData,int size, int nY4M);
    void render(const void *data, size_t size,int width,int height);
    void setdata(const void *data, size_t size,int width,int height);
    int  setRGBAData(int width, int height);
private:
    sp<IGraphicBufferProducer> inputProducer;
    sp<IGraphicBufferConsumer> inputConsumer;
    sp<StreamSplitter> splitter;
    sp<Surface>  surface;
    sp<ANativeWindow> window;
    int width ;
    int height ;
};

#endif //SURFACE_SOURCE_H