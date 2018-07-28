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

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "AccGyrSensor.h"

/*****************************************************************************/

/* The SENSORS Module */
static const struct sensor_t sSensorList[] = {
    {"Spich Accelerometer sensors", "Iris", 1, SENSORS_ACCELERATION_HANDLE,
     SENSOR_TYPE_ACCELEROMETER, 10240.0f, 1.0f, 0.5f, 10000, 0, 124,
     SENSOR_STRING_TYPE_ACCELEROMETER, "", 200000, SENSOR_FLAG_CONTINUOUS_MODE, {}},

    {"Spich Gyroscope sensors", "Iris", 1, SENSORS_GYROSCOPE_HANDLE,
     SENSOR_TYPE_GYROSCOPE, 2000.0f, 1.0f, 0.5f, 10000, 0, 124,
     SENSOR_STRING_TYPE_GYROSCOPE, "", 200000, SENSOR_FLAG_CONTINUOUS_MODE, {}},
};


static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
        *list = sSensorList;
        return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
        .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        .common = {
                .tag = HARDWARE_MODULE_TAG,
                .version_major = 1,
                .version_minor = 0,
                .id = SENSORS_HARDWARE_MODULE_ID,
                .name = "IRIS Sensor module",
                .author = "IRIS Inc",
                .methods = &sensors_module_methods,
                .dso  = NULL,
                .reserved = {0},
        },
        .get_sensors_list = sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int setDelay_sub(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        acc_gyr          = 0,
        numSensorDrivers,
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

	/* These function will be different depends on
	 * which sensor is implemented in AKMD program.
	 */
    int handleToDriver(int handle);
	int proxy_enable(int handle, int enabled);
	int proxy_setDelay(int handle, int64_t ns);
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    mSensors[acc_gyr] = new AccGyrSensor();
    mPollFds[acc_gyr].fd = mSensors[acc_gyr]->getFd();
    mPollFds[acc_gyr].events = POLLIN;
    mPollFds[acc_gyr].revents = 0;

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        mSensors[i]->close() ;
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::handleToDriver(int handle) {
	switch (handle) {
		case ID_A://acc			
        case ID_GY :
			return acc_gyr;
	}
	return -EINVAL;
}

int sensors_poll_context_t::activate(int handle, int enabled) {
	int drv = handleToDriver(handle);
	int err;
	if (drv == -EINVAL)
		return -EINVAL ;

	err = mSensors[drv]->setEnable(handle, enabled);

    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {
	return setDelay_sub(handle, ns);
}

int sensors_poll_context_t::setDelay_sub(int handle, int64_t ns) {
	int drv = handleToDriver(handle);
	int en = mSensors[drv]->getEnable(handle);
	int64_t cur = mSensors[drv]->getDelay(handle);
	int err = 0;

	if (en <= 1) {		
	/* no dependencies */
		if (cur != ns) {
			err = mSensors[drv]->setDelay(handle, ns);
		}
	} else {		
	/* has dependencies, choose shorter interval */
		if (cur > ns) {
			err = mSensors[drv]->setDelay(handle, ns);
		}
	}
	return err;
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int nb , polltime = 200;

    nb = poll(mPollFds, numFds, polltime);
    ALOGE_IF(0, "poll nb=%d, count=%d, pt=%d", nb, count, polltime);
    if (nb > 0) {
        for (int i = 0 ; count && i < numSensorDrivers ; ++i) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                nb = sensor->readEvents(data, count);
                mPollFds[i].revents = 0;
                if (nb <= 0)
                    continue ;
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (mPollFds[wake].revents & POLLIN) {
            char msg;
            int result = read(mPollFds[wake].fd, &msg, 1);
            ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
            ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
            mPollFds[wake].revents = 0;
        }

    }
    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
        int status = -EINVAL;
        sensors_poll_context_t *dev = new sensors_poll_context_t();

        memset(&dev->device, 0, sizeof(sensors_poll_device_t));

        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = 0;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = poll__close;
        dev->device.activate        = poll__activate;
        dev->device.setDelay        = poll__setDelay;
        dev->device.poll            = poll__poll;

        *device = &dev->device.common;
        status = 0;

        return status;
}




