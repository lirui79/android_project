#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <sys/resource.h>

#include "DisplayTest.h"

using namespace android;

int main() {
	setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);

	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	sp<DisplayTest> test = new DisplayTest();

	IPCThreadState::self()->joinThreadPool();

	return 0;
}
