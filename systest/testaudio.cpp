#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <binder/Parcel.h>
#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <media/IAudioFlinger.h>
#include <media/AudioSystem.h>

#include <utils/Log.h>

//#include <platform/IMConfigState.h>
#include "ConfigState.h"

using namespace android ;
using namespace iris ;

void test_audio_volume() {
    sp<IBinder> binder = defaultServiceManager()->getService(String16("media.audio_flinger"));
    if (binder == NULL) {
       printf("file[%s]:line[%d] binder == NULL test_audio_volume fail \n" , __FILE__ , __LINE__) ;
       return ;
    }
    sp<IAudioFlinger> audioFlinger = interface_cast<IAudioFlinger>(binder);
    if (audioFlinger == NULL) {
       printf("file[%s]:line[%d] audioFlinger == NULL test_audio_volume fail \n" , __FILE__ , __LINE__) ;
       return ;
    }

    float fVolume0 = audioFlinger->masterVolume();
    float fVolume1 = audioFlinger->streamVolume(AUDIO_STREAM_ALARM , AUDIO_IO_HANDLE_NONE) ;
    float fVolume2 = audioFlinger->streamVolume(AUDIO_STREAM_RING , AUDIO_IO_HANDLE_NONE) ;
    printf("get masterVolume %f streamVolume %f streamVolume %f \n" , fVolume0 , fVolume1 , fVolume2) ;

    int nIndex ;
    status_t code =  AudioSystem::getStreamVolumeIndex(AUDIO_STREAM_SYSTEM , &nIndex , AUDIO_DEVICE_OUT_DEFAULT) ;
    printf("get getStreamVolumeIndex return code %d  %d \n" , code , nIndex) ;
    code =  AudioSystem::getStreamVolumeIndex(AUDIO_STREAM_MUSIC , &nIndex , AUDIO_DEVICE_OUT_DEFAULT) ;
    printf("get getStreamVolumeIndex return code %d  %d \n" , code , nIndex) ;
}

void test_audio_volume_config() {
    int nRate = ConfigState::singleton()->getAudioRate() ;
    printf("file[%s]:line[%d] audio_volume Rate[%d] \n" , __FILE__ , __LINE__ , nRate) ;
}
