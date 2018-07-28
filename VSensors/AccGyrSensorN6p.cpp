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
#include <stdlib.h>
#include <sys/ioctl.h>
#include <cutils/log.h>

#include "AccGyrSensor.h"

#include "VRMath.h"




#define REVERSE32(x) __builtin_bswap32(x)
#define REVERSE64(x) __builtin_bswap64(x)


/******************************************************************************/

void AccGyrSensor::init_n6p_data()
{
    head_acc = 0;
    tail_acc = 0;
    head_gyr = 0;
    tail_gyr = 0;
    init = true;
    max_stamp = -200;//time should be positive
    id = 0;
    t = 0;
    last = 0;
    last_timestamp = 0;
    remaining_len = 0;
}

int AccGyrSensor::readSensorconfig()
{
	ALOGI("read sensorcal.json");
    float tmp = -1.52f;

    //read bias from json file
    int senfd = open("/persist/sensorcal.json", O_RDONLY);
    if (senfd==-1) ALOGI(strerror(errno));
    char senbuf[200];
    ::read(senfd, senbuf, sizeof(senbuf));
    ::close(senfd);
	
    ALOGI("senbuf, %s", senbuf);
    char senaccel[3][10], sengyro[3][10];
    char senbarometer[10];
    sscanf(senbuf, "%*[^(0-9|-)]%[^(,|\n| )]%*[^(0-9|-)]%[^(,|\n| )]%*[^(0-9|-)]%[^(,|\n| )]%*[^(0-9|-)]%[^(,|\n| )]%*[^(0-9|-)]%[^(,|\n| )]%*[^(0-9|-)]%[^(,|\n| )]%*[^(0-9|-)]%[^(,|\n| )]",
           senaccel[0], senaccel[1], senaccel[2], senbarometer, sengyro[0], sengyro[1], sengyro[2]);
    ALOGI("senbuf, %s %s %s %s %s %s %s",
           senaccel[0], senaccel[1], senaccel[2], senbarometer, sengyro[0], sengyro[1], sengyro[2]);

    //set_bias
    tx_buffer.args.accel_bias[0] = REVERSE32(atol(senaccel[0]));
    tx_buffer.args.accel_bias[1] = REVERSE32(atol(senaccel[1]));
    tx_buffer.args.accel_bias[2] = REVERSE32(atol(senaccel[2]));
    tx_buffer.args.gyro_bias[0] = REVERSE32(atol(sengyro[0]));
    tx_buffer.args.gyro_bias[1] = REVERSE32(atol(sengyro[1]));
    tx_buffer.args.gyro_bias[2] = REVERSE32(atol(sengyro[2]));
    tmp = float(atof(senbarometer));
    tx_buffer.args.barometer_bias = REVERSE32(*((int32_t*)&tmp));
	return 0 ;
}


void AccGyrSensor::pushback(int batch, double timestamp, float c0, float c1, float c2,
                                      double lm, double syst)
{
    switch(batch) 
	{
		case 1 :
		    if ((tail_acc + 1)%CACHESIZE == head_acc){
		       //ALOGD("full of cache_acc, h %d, t %d, %.0lf", head_acc, tail_acc, timestamp);
		        return;
		    }
		    cache_acc[tail_acc].time = timestamp;
		    cache_acc[tail_acc].vec[0] = c0;
		    cache_acc[tail_acc].vec[1] = c1;
		    cache_acc[tail_acc].vec[2] = c2;
		    cache_acc[tail_acc].lime = lm;
		    cache_acc[tail_acc].systime = syst;
		    tail_acc = (tail_acc + 1) % CACHESIZE;
			break ;

        case 2: 
		    if ((tail_gyr + 1)%CACHESIZE == head_gyr){
		        //ALOGD("full of cache_gyr, h %d, t %d, %.0lf", head_gyr, tail_gyr, timestamp);
		        return;
		    }
		    cache_gyr[tail_gyr].time = timestamp;
		    cache_gyr[tail_gyr].vec[0] = c0;
		    cache_gyr[tail_gyr].vec[1] = c1;
		    cache_gyr[tail_gyr].vec[2] = c2;
		    cache_gyr[tail_gyr].lime = lm;
		    cache_gyr[tail_gyr].systime = syst;
		    tail_gyr = (tail_gyr + 1) % CACHESIZE;
			break ;
	}
}


void AccGyrSensor::batch(int fd , int batch, int interval)
{
    // SET_BATCH
    memset(&tx_buffer.args, 0, sizeof tx_buffer.args);
    tx_buffer.cmd = CMDS::SET_BATCH;
    tx_buffer.args.batch_num_set = REVERSE32(batch);
    tx_buffer.args.flags = 0;
    tx_buffer.args.max_report_latency = 0;
    tx_buffer.args.sampling_interval = REVERSE32(interval);
    if (ioctl(fd, SPI_IOC_TXRX, &ioc_arg) < 0) 
		ALOGE("ioctl TxRx SET_BATCH(cmd=3) error");
    else 
		processData(&rx_buffer);

    // ENABLE_BATCH
    memset(&tx_buffer.args, 0, sizeof tx_buffer.args);
    tx_buffer.cmd = CMDS::ENABLE_BATCH;
    tx_buffer.args.batch_num_enable = REVERSE32(batch);
    tx_buffer.args.enable = 1;
    if (ioctl(fd, SPI_IOC_TXRX, &ioc_arg) < 0)
		ALOGE("ioctl TxRx ENABLE_BATCH(cmd=1) error");
    else 
		processData(&rx_buffer);
}

int  AccGyrSensor::setInitialStat()
{
	init_n6p_data() ;
	
	if (dev_fd == -1)
	{
		ALOGE(strerror(errno));
		return -EINVAL;
	}

	// SPI  WR_MODE
	uint8_t mode = 0 ;
	if (ioctl(dev_fd, SPI_IOC_WR_MODE, &mode) < 0) 
		ALOGE(strerror(errno));
	
	// WR LSB FIRST
	uint8_t lsb = 0 ;
	if (ioctl(dev_fd, SPI_IOC_WR_LSB_FIRST, &lsb) < 0)
		ALOGE(strerror(errno));

	// WR BITS PERR WORD
	uint8_t bits_per_word = 8 ;
	if (ioctl(dev_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) == -1)
		ALOGE(strerror(errno));

	// WR MAX SPEED HZ
	uint32_t speed_hz = 4800000 ;
	if (ioctl(dev_fd, SPI_IOC_WR_MAX_SPEED_HZ , &speed_hz) == -1)
		ALOGE(strerror(errno));

	// ENABLE TIMESTAMPS
	uint8_t enable = 1 ;
	if (ioctl(dev_fd, SPI_IOC_ENABLE_TIMESTAMPS, &enable) == -1)
		ALOGE(strerror(errno));

	// RESET HUB
	uint8_t zero = 0 ;
	if (ioctl(dev_fd, SPI_IOC_RESET_HUB, &zero) == -1)
		ALOGE(strerror(errno));

    ALOGI("Navigation : mode = %d lsb = %d bits = %d hz = %d \n" , mode , lsb , bits_per_word , speed_hz) ;

	readSensorconfig() ;
	
	sleep(1) ;

    ioc_arg.tx_buf = (unsigned long) (&tx_buffer);
    ioc_arg.rx_buf = (unsigned long) (&rx_buffer);
    ioc_arg.len = PACKAGE_SIZE;
    ioc_arg.speed_hz = speed_hz;
    ioc_arg.delay_usecs = 0;
    ioc_arg.bits_per_word = bits_per_word;
    ioc_arg.cs_change = 0;
    ioc_arg.pad = 0;

    memset(&tx_buffer, 0, sizeof tx_buffer);
    tx_buffer.magic_num = 85;
    // SET_BIAS
    tx_buffer.cmd = CMDS::SET_BIAS;


    if (ioctl(dev_fd, SPI_IOC_TXRX, &ioc_arg) < 0) 
		ALOGE(strerror(errno));
    else 
		processData(&rx_buffer);

    batch(dev_fd , 1, 2500000); // accel, 400Hz

    batch(dev_fd , 2, 2500000); // gyro, 400Hz

	memset(&tx_buffer.args, 0, sizeof tx_buffer.args);
    tx_buffer.cmd = CMDS::READ;
	return 0 ;
}

int  AccGyrSensor::processDataCache(RxBuffer *prx_buffer)
{
    if (prx_buffer->magic_num != 51) //check rx magic number
    {
       ALOGE("sensorhub sent bad start symbol[%d]\n" , prx_buffer->magic_num);
       return -1 ;
    }

    int16_t len = prx_buffer->len_hi << 8 | prx_buffer->len_lo, pos = 0, last_pos = 0;
    //PRINTF("len = %d\n",len);
    if (len > 0x100 || len <= 0)
    { //       ALOGE("sensorhub sent invalid transfer length[%d]\n" , len);
	    return 0 ;
    }

    uint8_t *data = prx_buffer->data, *now, cmd;
    if (remaining_len > 0)
    {
        memcpy(rx_cache + remaining_len, prx_buffer->data, len);
        len += remaining_len;
        data = rx_cache;
    }

    //inner cache
    std::list<VecData>  inner_cache ;
    std::list<int>      batchcache ;
    clock_gettime(CLOCK_MONOTONIC, &time_s);
    double syst = (time_s.tv_sec * 1000000.0) + time_s.tv_nsec / 1000.0;

    while (pos < len)
    {
        last_pos = pos;
        now = data + pos;
        cmd = *now;
        switch (cmd) // cmd
        {
            case 1: // change current batch number, package size 2, uint8_t|uint8_t
                if ((pos += 2) <= len)
                {
                    now_batch = now[1];
#ifdef DEBUG_LOG
                    ALOGI("Navigation: change batch: num %d\n", now_batch);
#endif
                }
                break;
            case 2: // set time, package size 9, uint8_t|uint64_t
                if ((pos += 9) <= len)
                {
                    timestamp = REVERSE64(*((uint64_t*)(now + 1)));
                    double ts = double(timestamp);
                    if (init){
                        init_stamp = ts;
                        init_sys_time = syst;
                        init = false;
                    }
                    ALOGI("Navigation: get time: %.0lf us\n", double(timestamp));
                }
                break;
            case 4: // report sensor data, package size 15, uint8_t|int16_t|float|float|float
                if ((pos += 15) <= len)
                {
                    SensorPackage *sensor = (SensorPackage*) now;
                    int16_t delta_time = (sensor -> time_hi << 8) | sensor -> time_lo;
                    timestamp += delta_time;

                    double ts = double(timestamp);
                    float c0 = TOFLOAT(REVERSE32(sensor -> x));
                    float c1 = TOFLOAT(REVERSE32(sensor -> y));
                    float c2 = TOFLOAT(REVERSE32(sensor -> z));

                    if (ts > max_stamp)
                        max_stamp = ts;
                    VecData  vecData ;
                    vecData.time = ts ;
                    vecData.vec[0] = c0;
                    vecData.vec[1] = c1;
                    vecData.vec[2] = c2;
                    inner_cache.push_back(vecData) ;
                    batchcache.push_back(now_batch) ;              
                }

                break;
            case 5: // package size 19, uint8_t|uint16_t|int32_t|int32_t|int32_t|int32_t
                if ((pos += 19) <= len)
                {
                    uint16_t a = now[1] << 8 | now[2];
                    int32_t b = REVERSE32(*(int32_t*)(now + 3));
                    int32_t c = REVERSE32(*(int32_t*)(now + 7));
                    int32_t d = REVERSE32(*(int32_t*)(now + 11));
                    int32_t e = REVERSE32(*(int32_t*)(now + 15));
                    ALOGI("cmd5 %d %d %d %d %d\n", a, b, c, d, e);
                }
                break;
            case 6: // package size 3, uint8_t|uint8_t|uint8_t
                if ((pos += 3) <= len)
                {
                    ALOGI("cmd6 %d %d\n", now[1], now[2]);
                }
                exit(0);
                break;
            case 7: // package size 13, uint8_t|int32_t|int32_t|int32_t
                if ((pos += 13) <= len)
                {
                    int32_t a = REVERSE32(*(int32_t*)(now + 1));
                    int32_t b = REVERSE32(*(int32_t*)(now + 5));
                    int32_t c = REVERSE32(*(int32_t*)(now + 9));
                    ALOGI("cmd7 %d %d %d\n", a, b, c);
                }
                break;
            case 8: // package size 2, uint8_t|uint8_t
               if ((pos += 2) <= len)
                    ALOGI("cmd8 %d\n", now[1]);
               break;
            case 9: // package size 13, uint8_t|int32_t|int32_t|int32_t
               if ((pos += 13) <= len)
                {
                    int32_t a = REVERSE32(*(int32_t*)(now + 1));
                    int32_t b = REVERSE32(*(int32_t*)(now + 5));
                    int32_t c = REVERSE32(*(int32_t*)(now + 9));
                    ALOGI("Navigation: cmd9 %d %d %d\n", a, b, c);
                }
                break;
           case 10: // package size 2, uint8_t|uint8_t
                if ((pos += 2) <= len)
                    ALOGI("cmd10 %d\n", now[1]);
                break;
            case 255: // notice, uint8_t|uint8_t|string
                if ((pos += 3) <= len)
                {
                    uint8_t str_len = now[2];
                    if ((pos += str_len) <= len)
                    {
                        now += 3;
                        ALOGI("sensorhub said:'");
                        while (str_len)
                        {
                            ALOGI("%c",*now);
                            str_len--;
                            now++;
                        }
                        ALOGI("'");
                    }
                }
                break;
            default: //ignore
                ALOGI("should not be here pos = %d\n", pos);
                ++pos;
                exit(0);
                break;
        }
    }
	
	remaining_len = 0;
    if (pos > len)
    {
        remaining_len = len - last_pos;
        ALOGE("Detect incomplete package. Remaining %d bytes, needs %d more bytes\n", remaining_len, pos - len);
        memcpy(rx_cache, data + last_pos, remaining_len);
    }
		
    double scalek = (syst-init_sys_time)/(max_stamp-init_stamp);

    std::list<VecData>::iterator it = inner_cache.begin() ;
    std::list<int>::iterator ite = batchcache.begin() ;
    for( ; it != inner_cache.end(); ++it , ++ite){
        pushback(*ite , init_sys_time + scalek*(it->time - init_stamp), it->vec[0],
                        it->vec[1],it->vec[2], it->time, syst);
    }

	return 1 ;
}

int  AccGyrSensor::processData(RxBuffer *prx_buffer)
{
    int nCode = processDataCache(prx_buffer);
    if (nCode <= 0)
       return nCode ;

    float qua_bias[4];
    qua_bias[0] = 0;
    qua_bias[1] = 0.70710678f;
    qua_bias[2] = 0.70710678f;
    qua_bias[3] = 0;
    iris::Mathf::qua_norm(qua_bias, qua_bias);
	
	int numEventReceived = 0 , nfind = 0 ;
	
	VecData acc , gyr ;
	while(head_acc != tail_acc && head_gyr != tail_gyr)
	{
	    double t_acc = (cache_acc[head_acc].lime)/ 1000.;   //lm to align
	    double t_gyr = (cache_gyr[head_gyr].lime)/ 1000.;

	    if (t_acc - t_gyr > 1.){
	        head_gyr = (head_gyr + 1) % CACHESIZE;
               //ALOGI("t_acc[%f] - t_gyr[%f] > 1." , t_acc , t_gyr);
	        continue;
	    } 

	   if (t_acc - t_gyr < -1.){
              //ALOGI("t_acc[%f] - t_gyr[%f] < -1." , t_acc , t_gyr);
	        head_acc = (head_acc + 1) % CACHESIZE;
	        continue;
	    } 

        struct timespec sysnow = {0, 0};
        clock_gettime(CLOCK_MONOTONIC, &sysnow);
        double nowtime = (sysnow.tv_sec * 1000000.0) + sysnow.tv_nsec / 1000.0;
        
       // if (id++ % 1000 == 200)
       //     ALOGI("Th, %0.lf, %.0lf", nowtime, t_acc);
	          
	    nfind = 1 ; 
		acc = cache_acc[head_acc] ;
		head_acc = (head_acc + 1) % CACHESIZE;	
		gyr = cache_gyr[head_gyr] ;
		head_gyr = (head_gyr + 1) % CACHESIZE;					
    }
    
    if(nfind == 0)
       return 0 ;

    ///ALOGI("find sensors data");
    double t_acc = acc.time / 100.;  // 0.1ms now 10/21
    double t_gyr = gyr.time / 100.;	
    for (int j = 0 ; j < numSensors ; ++j)
    {
        if (mEnabled[0] == 0 && mEnabled[1] == 0) 
            continue ;
        sensors_event_t events ;
        memset(&events, 0, sizeof(events));
        events.timestamp = static_cast<uint64_t>(t_acc + (t_gyr - t_acc) * 0.5) * 100000;
        if ((j % numSensors) == 0)
        {
            events.version = sizeof(sensors_event_t);
            events.sensor = ID_A;
            events.type = SENSOR_TYPE_ACCELEROMETER;
            events.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
            events.acceleration.v[0] = acc.vec[1] ;
            events.acceleration.v[1] = acc.vec[0] ;
            events.acceleration.v[2] = -acc.vec[2] ;

            iris::Mathf::vec_rotate(events.acceleration.v, qua_bias, events.acceleration.v); 
        } else {
            events.version = sizeof(sensors_event_t);
            events.sensor = ID_GY;
            events.type = SENSOR_TYPE_GYROSCOPE;
            events.gyro.status = SENSOR_STATUS_ACCURACY_HIGH;
	        events.gyro.v[0] = gyr.vec[1];
	        events.gyro.v[1] = gyr.vec[0];
	        events.gyro.v[2] = -gyr.vec[2];
	        iris::Mathf::vec_rotate(events.gyro.v, qua_bias, events.gyro.v);
        }

        pthread_mutex_lock(&mMutex_t) ;
        if (mEvents.size() >= 32)
            mEvents.pop_front() ;
        mEvents.push_back(events) ;
        pthread_mutex_unlock(&mMutex_t) ;
        ++numEventReceived;
    }     
 
	return numEventReceived ;
}

int  AccGyrSensor::readData()
{
    struct pollfd mPollFds[1];
    mPollFds[0].fd = dev_fd;
    mPollFds[0].events = POLLIN;
    mPollFds[0].revents = 0;
    const int inter = 100;
    int nECount = 0 ;

    while(mExit == 0) {
        int nCode = poll(mPollFds, 1, 1000);
        if(nCode <= 0) {
            ALOGE("poll return code[%d] errno[%d]" , nCode , errno);
            ++nECount ;
            if (nECount <= 20)
                continue ;
            nECount = 0 ;
            close_device() ;
            open_device() ;
            setInitialStat();
            mPollFds[0].fd = dev_fd;
            mPollFds[0].events = POLLIN;
            mPollFds[0].revents = 0;
            continue ;
        }

        memset(&tx_buffer.args, 0, sizeof tx_buffer.args);
        tx_buffer.cmd = CMDS::READ;
        int numEventReceived = 0;
        nCode = ioctl(dev_fd, SPI_IOC_TXRX, &ioc_arg) ;
        if (nCode < 0) {
            ALOGE("ioctl TxRx READ(cmd=0) return code[%d] errno[%d]" , nCode , errno);
            continue ;
        }  else {
            numEventReceived = processData(&rx_buffer);
        }

        if (numEventReceived < 0)
        {
            ++nECount ;
            if (nECount <= 20)
                continue ;
            nECount = 0 ;
            close_device() ;
            open_device() ;
            setInitialStat();
            mPollFds[0].fd = dev_fd;
            mPollFds[0].events = POLLIN;
            mPollFds[0].revents = 0;
            continue ;
        }

        if (numEventReceived > 0) {
            const char WAKE_MESSAGE = 'W';
            int result = write(mWritePipeFd, &WAKE_MESSAGE, 1);
            ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
        }

        if ((++t % inter) == 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &time_s);
            double time = time_s.tv_sec + time_s.tv_nsec / 1000000000.0;

     //      if (t > inter)
     //        ALOGD("%d freq = %.3lfHz freq2 = %.3lfHz\n",t,  inter / (time - last), inter / (((timestamp - last_timestamp) / 1000000.0) ));
            last = time;
            last_timestamp = timestamp;
        }
    }
    return 0 ;
}
