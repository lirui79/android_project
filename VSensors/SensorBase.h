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

#ifndef ANDROID_SENSOR_BASE_H
#define ANDROID_SENSOR_BASE_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <hardware/sensors.h>

#define   ID_A    0
#define   ID_GY   1 


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define SENSORS_ACCELERATION_HANDLE                (ID_A)
#define SENSORS_GYROSCOPE_HANDLE                   (ID_GY)
/*****************************************************************************/

struct sensors_event_t;

class SensorBase {
protected:
    const char* dev_name;
    const char* data_name;
    char        input_name[PATH_MAX];
    int         dev_fd;
    
    static int64_t getTimestamp();
    static int64_t timevalToNano(timeval const& t) {
        return t.tv_sec*1000000000LL + t.tv_usec*1000;
    }

    int open_device();
    int close_device();

public:
            SensorBase(
                    const char* dev_name,
                    const char* data_name);

    virtual ~SensorBase();

    virtual int close()  ;

    virtual int readEvents(sensors_event_t* data, int count) = 0;
    virtual bool hasPendingEvents() const;
    virtual int getFd() const;

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int64_t getDelay(int32_t handle);

	/* When this function is called, increments the reference counter. */
    virtual int setEnable(int32_t handle, int enabled) = 0;
	/* It returns the number of reference. */
    virtual int getEnable(int32_t handle) = 0;
};

/*****************************************************************************/

#endif  // ANDROID_SENSOR_BASE_H
