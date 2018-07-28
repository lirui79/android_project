/*
 * Copyright (C) 2017 Iris , Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <dlfcn.h>

#include <cutils/log.h>

#include "AccGyrSensor.h"


/*****************************************************************************/

AccGyrSensor::AccGyrSensor()
: SensorBase("/dev/spich", NULL)
{
	for (int i=0; i<numSensors; i++) {
		mEnabled[i] = 1;
		mDelay[i] = -1;
	}

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
	mReadPipeFd = wakeFds[0];
    mWritePipeFd = wakeFds[1];

    pthread_mutex_init(&mMutex_t , NULL) ;
	mExit = 0 ;
    open_device();

    setInitialStat();

	pthread_create(&(mThread_t), NULL, &readSensorThread, this);
}

AccGyrSensor::~AccGyrSensor()
{
	for (int i=0; i<numSensors; i++) {
		setEnable(i, 0);
	}

	close() ;
	close_device();

    ::close(mReadPipeFd);
    ::close(mWritePipeFd);
	pthread_mutex_destroy(&mMutex_t) ;
}

int AccGyrSensor::close() {
    pthread_mutex_lock(&mMutex_t) ;
    mExit = 1 ;
    pthread_mutex_unlock(&mMutex_t) ;
	close_device();
	return 0 ;
}

bool AccGyrSensor::hasPendingEvents() const {
	return false ;
}

int AccGyrSensor::setEnable(int32_t handle, int enabled) {
	int id = handle2id(handle);
	int err = 0;
	char buffer[2];

	switch (id) {
	case Acc:

		break;
	case Gyr:

		break;
	default:
		ALOGE("AccGyrSensor: unknown handle (%d)", handle);
		return -EINVAL;
	}


	if (mEnabled[id] <= 0) {
		if(enabled) mEnabled[id] = 1 ;
	} else if (mEnabled[id] == 1) {
		if(!enabled) mEnabled[id] = 0 ;
	}

    return err;
}

int AccGyrSensor::setDelay(int32_t handle, int64_t ns)
{
	int id = handle2id(handle);
	int err = 0;
	char buffer[32];
	int bytes;

    if (ns < -1 || 2147483647 < ns) {
		ALOGE("AccGyrSensor: invalid delay (%lld)", ns);
        return -EINVAL;
	}

	switch (id) {
	case Acc:

		break;
	case Gyr:

		break;
	default:
		ALOGE("AccGyrSensor: unknown handle (%d)", handle);
		return -EINVAL;
	}

	if (ns != mDelay[id]) {
 
			mDelay[id] = ns;
			ALOGD("AccGyrSensor: set %d to %f ms.", id , ns/1000000.0f);
	}
	
    return err;
}

int AccGyrSensor::getFd() const {
	return mReadPipeFd ;
}

int64_t AccGyrSensor::getDelay(int32_t handle)
{
	int id = handle2id(handle);
	if (id >= 0) {
		return mDelay[id];
	} else {
		return 0;
	}
}

int AccGyrSensor::getEnable(int32_t handle)
{
	int id = handle2id(handle);
	if (id >= 0) {
		return mEnabled[id];
	} else {
		return 0;
	}
}

int AccGyrSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

	char msg;
	int result = read(mReadPipeFd , &msg, 1);
	if (result < 0) {
		ALOGE("error reading from wake pipe (%s)", strerror(errno));
		return -EINVAL;
	}
	const char WAKE_MESSAGE = 'W';
	if (msg != WAKE_MESSAGE) {
		ALOGE("unknown message on wake queue (0x%02x)", int(msg));
		return -EINVAL;
	}

	std::list<sensors_event_t> Events ;
    pthread_mutex_lock(&mMutex_t) ;
	if (count >= mEvents.size()) {
		Events = mEvents ;
		mEvents.clear() ;
	} else {
		std::list<sensors_event_t>::iterator it = mEvents.begin() ;
		for ( ; count > 0 ; --count , ++it) {
			Events.push_back(*it) ;
		}
		mEvents.erase(mEvents.begin() , it) ;
	}

    pthread_mutex_unlock(&mMutex_t) ;
	int nCount = 0 ;
	for (std::list<sensors_event_t>::iterator it = Events.begin() ; it != Events.end() ; ++it , ++nCount , ++data)	{
		*data = *it ;
	}

	return nCount ;
}

int AccGyrSensor::handle2id(int32_t handle)
{
    switch (handle) {
        case ID_A:
			return Acc;
        case ID_GY:
			return Gyr;
		default:
			ALOGE("AccGyrSensor: unknown handle (%d)", handle);
			return -EINVAL;
    }
}

