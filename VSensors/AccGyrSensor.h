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

#ifndef ANDROID_ACC_GYR_SENSOR_H
#define ANDROID_ACC_GYR_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <pthread.h>
#include <list>

#include "spi-contexthub.h"
#include "SensorBase.h"

/*****************************************************************************/

class AccGyrSensor : public SensorBase {
private:
    static const int PACKAGE_SIZE = 0x103;
    uint8_t  now_batch ;
    
    enum CMDS
    {
        READ = 0,
        ENABLE_BATCH = 1,
        SET_BATCH = 3,
        SET_BIAS = 8
    };

    struct VecData{
        double time;
        float vec[3];
        double lime;
        double systime;
    };


#pragma pack(push)
#pragma pack(1)
    struct TxBuffer //all the following data are big ending, needs transfer
    {
        int8_t magic_num; // should be 85
        int32_t time1;
        int32_t time2;
        int8_t cmd;
        union
        {
            struct // cmd = ENABLE_BATCH
            {
                int32_t batch_num_enable;
                uint8_t enable;
            };
            struct // cmd = SET_BATCH
            {
                int32_t batch_num_set;
                int32_t flags;
                int32_t max_report_latency; //ns
                int32_t sampling_interval; //ns
            };
            struct // cmd = SET_BIAS
            {
                int32_t accel_bias[3];
                int32_t gyro_bias[3];
                int32_t barometer_bias; //reversed float
            };
            uint8_t raw[PACKAGE_SIZE - 10];
        }args;
    };

    struct RxBuffer
    {
        uint8_t magic_num; // should be 51
        uint8_t len_hi;
        uint8_t len_lo;
        uint8_t data[PACKAGE_SIZE - 3];
    };

    struct SensorPackage // big ending, size = 15
    {
        uint8_t cmd; // should be 4
        uint8_t time_hi;
        uint8_t time_lo;
        uint32_t x,y,z; //reversed float
    };
#pragma pack(pop)

    TxBuffer tx_buffer;
    
    RxBuffer rx_buffer;

    uint64_t timestamp;
    
    uint16_t remaining_len ;
    
    uint8_t rx_cache[1025];
    
    struct timespec time_s = {0, 0};
    
    const int SENSOR_PACKAGE_SIZE = sizeof(SensorPackage);

    static const int CACHESIZE = 100;
    VecData cache_acc[CACHESIZE];
    int head_acc;
    int tail_acc;
    VecData cache_gyr[CACHESIZE];
    int head_gyr;
    int tail_gyr;

    bool init;
    double max_stamp;
    double init_stamp;
    double init_sys_time;
    int id;


    int t ;

    double last ;

    uint64_t last_timestamp ;

    const int inter = 100;

    struct spi_ioc_transfer ioc_arg;

public:
    AccGyrSensor();
    virtual ~AccGyrSensor();

    virtual int close() ;

    enum {
		Acc = 0,
        Gyr = 1,
        numSensors
    };

    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual int64_t getDelay(int32_t handle);
    virtual int getEnable(int32_t handle);
    virtual bool hasPendingEvents() const;
    virtual int getFd() const;

    static void* readSensorThread(void *arg) {
        ((AccGyrSensor*)arg)->readData() ;
        return NULL ;
    }
private:
    void init_n6p_data() ;
    int  readSensorconfig();
    void pushback(int batch, double timestamp, float c0, float c1, float c2,
                                          double lm, double syst);
    static float TOFLOAT(int a)
    {
        return *((float*)(&a));
    }


    void batch(int fd , int batch, int interval) ;

    int  setInitialStat() ;

    int  readData() ;

    int  processData(RxBuffer *prx_buffer) ;

    int  processDataCache(RxBuffer *prx_buffer);
	 
    pthread_mutex_t   mMutex_t ;
    pthread_t         mThread_t ;
    int               mWritePipeFd ;
    int               mReadPipeFd ;
    int               mExit ;
    int mEnabled[numSensors];
	int64_t mDelay[numSensors];

    std::list<sensors_event_t> mEvents ;

	int handle2id(int32_t handle);
};

/*****************************************************************************/

#endif  // ANDROID_ACC_GYR_SENSOR_H
