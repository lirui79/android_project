
#ifndef ANDROID_DISPLAYTEST_H
#define ANDROID_DISPLAYTEST_H

#include <binder/IBinder.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <gui/Surface.h>
#include <utils/Thread.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

namespace android {

class DisplayTest : public Thread {
   public:
	DisplayTest();
	virtual ~DisplayTest();
	void initializeSensor();

   private:
	virtual bool threadLoop();
	virtual status_t readyToRun();
	virtual void onFirstRef();

	int mWidth;
	int mHeight;
	EGLDisplay mDisplay;
	EGLDisplay mContext;
	EGLDisplay mSurface;
	sp<SurfaceControl> mFlingerSurfaceControl;
	sp<Surface> mFlingerSurface;
	int led;
	bool cpuSet;
};
}

#endif
