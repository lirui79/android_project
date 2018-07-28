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
#include <batteryservice/BatteryService.h>
#include <batteryservice/IBatteryPropertiesListener.h>
#include <batteryservice/IBatteryPropertiesRegistrar.h>
#include <utils/Log.h>

//#include <platform/IMConfigState.h>
#include "ConfigState.h"

using namespace android ;

class BnBatteryPropertiesListener : public BnInterface<IBatteryPropertiesListener> {
public:
     BnBatteryPropertiesListener() {}

     virtual ~BnBatteryPropertiesListener() {}


    virtual void batteryPropertiesChanged(struct BatteryProperties props) {
        ALOGI("batteryPropertiesChanged\n") ;
    }

    status_t publish() {
        return defaultServiceManager()->addService(String16("android.os.IBatteryPropertiesListener") , this) ;
    }

protected:
   virtual status_t onTransact( uint32_t code, const Parcel& data,Parcel* reply,uint32_t flags = 0){
          ALOGI("Client onTransact, code = %d", code);   
           switch (code){
           case TRANSACT_BATTERYPROPERTIESCHANGED:
                CHECK_INTERFACE(IBatteryPropertiesListener, data, reply);
                int n = data.readInt32();
                struct BatteryProperties pops;
                pops.chargerAcOnline = data.readInt32() == 1 ? true : false;
                pops.chargerUsbOnline = data.readInt32() == 1 ? true : false;
                pops.chargerWirelessOnline = data.readInt32() == 1 ? true : false;
                pops.maxChargingCurrent = data.readInt32();
                pops.batteryStatus = data.readInt32();
                pops.batteryHealth = data.readInt32();
                pops.batteryPresent = data.readInt32() == 1 ? true : false;
                pops.batteryLevel = data.readInt32();
                pops.batteryVoltage = data.readInt32();
                pops.batteryTemperature = data.readInt32();
                pops.batteryTechnology = String8((data.readString16()).string());
                //pops.readFromParcel(&data) ;
                batteryPropertiesChanged(pops) ;
                reply->writeInt32(OK) ;
                return OK;
           }
           return BBinder::onTransact(code, data, reply, flags);
     }

} ;

using namespace iris ;

void test_battery() {
    sp<IBinder> binder = defaultServiceManager()->getService(String16("batteryproperties")) ;
    if (binder != NULL) {
      printf("file[%s]:line[%d] getBinder batteryproperties success \n" , __FILE__ , __LINE__) ;
    } else {

      printf("file[%s]:line[%d] getBinder batteryproperties fail \n" , __FILE__ , __LINE__) ;
      return ;
    }

    sp<IBatteryPropertiesRegistrar> iBatteryProReg  = interface_cast<IBatteryPropertiesRegistrar>(binder);

    if (iBatteryProReg != NULL) {
      printf("file[%s]:line[%d] IBatteryPropertiesRegistrar batteryproperties success \n" , __FILE__ , __LINE__) ;
    } else {

      printf("file[%s]:line[%d] IBatteryPropertiesRegistrar batteryproperties fail \n" , __FILE__ , __LINE__) ;
      return ;
    }
    
    ProcessState::self()->startThreadPool();

    BnBatteryPropertiesListener* bnBattery = new BnBatteryPropertiesListener() ;
    //status_t ret = bnBattery->publish() ;
    sp<IBatteryPropertiesListener> IBnBattery(bnBattery) ;
    iBatteryProReg->registerListener(IBnBattery) ;//*/

    struct BatteryProperty batteryPorp ;
    status_t code = iBatteryProReg->getProperty(BATTERY_PROP_CURRENT_NOW , &batteryPorp) ;

    if (code == 0) {
      printf("file[%s]:line[%d] getProperty batteryproperties success  %d \n" , __FILE__ , __LINE__ , batteryPorp.valueInt64) ;
    } else {

      printf("file[%s]:line[%d] getProperty batteryproperties fail \n" , __FILE__ , __LINE__) ;
      return ;
    } 

    code = iBatteryProReg->getProperty(BATTERY_PROP_CAPACITY , &batteryPorp) ;

    if (code == 0) {
      printf("file[%s]:line[%d] getProperty batteryproperties success  %d \n" , __FILE__ , __LINE__ , batteryPorp.valueInt64) ;
    } else {

      printf("file[%s]:line[%d] getProperty batteryproperties fail \n" , __FILE__ , __LINE__) ;
      return ;
    } 

    ///*
    int nCount = 0 ;
    while(nCount++ < 50) {
        sleep(1) ;
    }

    iBatteryProReg->unregisterListener(bnBattery) ;//*/
}

void test_battery_config() {
    int nRate = ConfigState::singleton()->getBatteryRate() ;
    printf("file[%s]:line[%d] Battery Rate[%d] \n" , __FILE__ , __LINE__ , nRate) ;
}
