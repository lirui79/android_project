#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//#include <platform/IMConfigState.h>
#include "JavaCore/JavaCore.h"
using namespace iris ;


void test_wifi_config() {
    struct WiFiInfo wifiInfo ;
    std::string strWiFi = JavaCore::singleton()->getWifiInfo(&wifiInfo) ;   
    printf("file[%s]:line[%d] wifi[%s] \n" , __FILE__ , __LINE__ , strWiFi.c_str() ) ;

    printf("file[%s]:line[%d] Ssid[%s] Bssid[%s] Mac[%s] netid %d  state %d  rssi %d  speed %d strength %d \n" , __FILE__ , __LINE__ , wifiInfo.mSsid , wifiInfo.mBssid , wifiInfo.mMac , wifiInfo.mNet_ID , wifiInfo.mState , wifiInfo.mRssi , wifiInfo.mLink_speed , wifiInfo.mStrength) ;
}