
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <gui/Surface.h>
#include <ui/DisplayInfo.h>

#include <GLES/gl.h>
#include <EGL/egl.h>

#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/resource.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <time.h>

#include "DisplayTest.h"

namespace android {

DisplayTest::DisplayTest() : Thread(false) {
}

DisplayTest::~DisplayTest() {}

void DisplayTest::onFirstRef() { run("DisplayTest", PRIORITY_DISPLAY); }

void print_err() {
	int err = errno;
	fprintf(stderr, "openerr, errno= %s\n", strerror(err));
	exit(-1);
}

	sp<SurfaceComposerClient> mSession;
	sp<SurfaceControl> control;

status_t DisplayTest::readyToRun() {
	srand(time(NULL));
	led = open("/dev/ttyUSB0", O_RDWR);

	if (led == -1) {
		ALOGE("failed to open ttyUSB0");
	} else {
		ALOGD("open succeed");
	}

	int WIN_LAYER_WIDTH = 1920;
	int WIN_LAYER_HEIGHT = 1080;
	int WIN_LAYER_FORMAT = PIXEL_FORMAT_RGBA_8888;

	mSession = new SurfaceComposerClient();
	sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdHdmi));
	control = mSession->createSurface(
		String8("DisplayTest"), WIN_LAYER_WIDTH, WIN_LAYER_HEIGHT,
		PIXEL_FORMAT_RGBA_8888, ISurfaceComposerClient::eFXSurfaceNormal);


	sp<Surface> s = control->getSurface();

	printf("%d %d\n", WIN_LAYER_WIDTH, WIN_LAYER_HEIGHT);

	SurfaceComposerClient::setDisplayLayerStack(dtoken, 100);

	SurfaceComposerClient::openGlobalTransaction();
	control->setLayerStack(100);
	control->setLayer(0x40000000);
	control->setPosition(0, 0);
	control->show();
	SurfaceComposerClient::closeGlobalTransaction();

	// initialize opengl and egl
	const EGLint attribs[] = {EGL_RED_SIZE,  8, EGL_GREEN_SIZE, 8,
							  EGL_BLUE_SIZE, 8, EGL_NONE};
	EGLint w, h;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	surface = eglCreateWindowSurface(display, config, s.get(), NULL);
	context = eglCreateContext(display, config, NULL, NULL);
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		return NO_INIT;

	mDisplay = display;
	mContext = context;
	mSurface = surface;
	mWidth = w;
	mHeight = h;
	printf("%d %d\n", mWidth, mHeight);
	// mFlingerSurfaceControl =
	//	control;  // This line is important although nowhere is using it
	mFlingerSurface = s;

	// initializeSensor();

	eglSwapBuffers(mDisplay, mSurface);
	return NO_ERROR;
}

void DisplayTest::initializeSensor() {
	speed_t baud = B115200;
	struct termios settings;
	tcgetattr(led, &settings);

	if (cfsetospeed(&settings, baud) == -1) print_err(); /* baud rate */
	if (cfsetispeed(&settings, baud) == -1) print_err(); /* baud rate */
	settings.c_cflag &= ~PARENB;						 /* no parity */
	settings.c_cflag &= ~CSTOPB;						 /* 1 stop bit */
	settings.c_cflag &= ~CSIZE;
	settings.c_cflag |= CS8 | CLOCAL; /* 8 bits */
	settings.c_lflag = ICANON;		  /* canonical mode */
	settings.c_oflag &= ~OPOST;		  /* raw output */

	if (tcsetattr(led, TCSANOW, &settings) == -1)
		print_err(); /* apply the settings */
	if (tcflush(led, TCOFLUSH) == -1) print_err();
	fprintf(stderr, "initialize success\n");
}

bool DisplayTest::threadLoop() {
	static int i = 0;
	static double ds = 0;

	struct timespec ts, te, tn, td;
	// glEnable(GL_SCISSOR_TEST);

	// struct timespec s;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	glScissor(0, 0, 1200, 1200);

	switch (i % 3) {
		case 0:
			glClearColor(1, 0, 0, 1);
			break;
		case 1:
			glClearColor(0, 1, 0, 1);
			break;
		case 2:
			glClearColor(0, 0, 1, 1);
			break;
	}
	glClear(GL_COLOR_BUFFER_BIT);//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);   glViewport(0, 0, mWidth, mHeight);

	glFlush();
	glFinish();
#if !IRIS_VR_BUFFER
	eglSwapBuffers(mDisplay, mSurface);
#endif
#ifdef PLATFORM_VERSION_5
	eglSwapBuffers(mDisplay, mSurface);
#endif

    /*
	ALOGI("remainingTimeForLeft %.0lf\n",
		  (double)mVSyncCoord->remainingTimeForLeft());
	mVSyncCoord->waitForLeft();

	clock_gettime(CLOCK_MONOTONIC, &ts);
	int c = write(led, "1", 1);
	clock_gettime(CLOCK_MONOTONIC, &tn);

	//glScissor(0, 1200, 1200, 2400);

	switch (i % 3) {
		case 0:
			glClearColor(1, 0, 1, 1);
			break;
		case 1:
			glClearColor(1, 1, 0, 1);
			break;
		case 2:
			glClearColor(0, 1, 1, 1);
			break;
	}
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
    glFinish();
    */
    glFinish();
// clock_gettime(CLOCK_MONOTONIC, &te);
#ifdef PLATFORM_VERSION_5
	eglSwapBuffers(mDisplay, mSurface);
#endif
	eglSwapBuffers(mDisplay, mSurface);
	clock_gettime(CLOCK_MONOTONIC, &te);
	double diff_m =
		(tn.tv_sec - ts.tv_sec) * 1e3 + (tn.tv_nsec - ts.tv_nsec) / 1.0e6;
	double diff_a =
		(te.tv_sec - ts.tv_sec) * 1e3 + (te.tv_nsec - ts.tv_nsec) / 1.0e6;
	ds += diff_a;
	// ALOGD("Before glFlush(): %lfms", diff_m);
	// ALOGD("Client render time: %lfms", diff_a);

	++i;
	if (i % 120 == 0) {
		ALOGD("Average render time: %lf", ds / i);
		ds = 0;
		// i = 0;
	}

	return true;
}
}
