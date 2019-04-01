#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <utils/Log.h>

using namespace android;
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "sampleService"
#define SAMPLE_SERIVCE_DES "sample.hello"
#define SAMPLE_CB_SERIVCE_DES "android.os.SampleCallback"
#define SRV_CODE 1
#define CB_CODE 1
#define  ON_CHANGED 2
class SampleService: public BBinder {
public:
     SampleService() {
          mydescriptor = String16(SAMPLE_SERIVCE_DES);
     }
     
     virtual ~SampleService() {
     }

     virtual const String16& getInterfaceDescriptor() const {
           return mydescriptor;
     }

     status_t OnChanged(int Code) {
        if (callback == NULL)
            return -1 ;
        Parcel data, reply;
        data.writeInterfaceToken(String16(SAMPLE_CB_SERIVCE_DES));
        data.writeInt32(Code);
        //props.writeToParcel(&data);
        callback->transact(ON_CHANGED, data, &reply, IBinder::FLAG_ONEWAY);
        int32_t ret = reply.readInt32() ;
        return ret ;
     }
     
protected:
     
     void callFunction() {
          ALOGI("Service callFunction-----------");
     }

     virtual status_t onTransact(uint32_t code, const Parcel& data,
              Parcel* reply, uint32_t flags = 0) {
          ALOGI("Service onTransact, code = %d" , code);
           switch (code) {
           case SRV_CODE:
              callback = data.readStrongBinder();
              if (callback != NULL)
              {
                   Parcel _data, _reply;
                   _data.writeInterfaceToken(String16(SAMPLE_CB_SERIVCE_DES));
                    int ret = callback->transact(CB_CODE, _data, &_reply, 0);
              }
              callFunction();
               break;
           default:
               return BBinder::onTransact(code, data, reply, flags);
          }
           return 0;
     }
private:
     String16 mydescriptor;
     sp<IBinder> callback;
};

int main() {
     sp < IServiceManager > sm = defaultServiceManager();
     SampleService* samServ = new SampleService();
     status_t ret = sm->addService(String16(SAMPLE_SERIVCE_DES), samServ);
     ALOGI("Service addservice sample.hello %d" , ret);
     ProcessState::self()->startThreadPool();
//     IPCThreadState::self()->joinThreadPool( true);
     int nCount = 0 ;
     int nRet = 0 ;
     while(1){
         nRet = samServ->OnChanged(nCount++) ;
         sleep(1) ;
         ALOGI("Service alive nCount %d nRet %d" , nCount , nRet);
     }
     return 0;
}