#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <private/binder/binder_module.h>
#include <utils/Log.h>

using namespace android;
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "sampleCallback"
#define SAMPLE_SERIVCE_DES "sample.hello"
#define SAMPLE_CB_SERIVCE_DES "android.os.SampleCallback"
#define SRV_CODE 1
#define CB_CODE 1

#define  ON_CHANGED 2
class SampeCallback : public BBinder
{
     public:
     SampeCallback()
     {
          mydescriptor = String16(SAMPLE_CB_SERIVCE_DES);
     }
     virtual ~SampeCallback() {
          
     }
     virtual const String16& getInterfaceDescriptor() const{
           return mydescriptor;
     }
     protected:
     
     void callbackFunction() {
          ALOGI("Client callback function called-----------cb");
     }

     void OnChanged(int Code) {
          ALOGI("Client OnChanged function called Code %d-----------cb" , Code);
     }
     
     virtual status_t onTransact( uint32_t code,
               const Parcel& data,Parcel* reply,uint32_t flags = 0){
          ALOGI("Client onTransact, code = %d", code);   
         
           switch (code){
           case CB_CODE:
               if (!data.checkInterface(this))
                  return PERMISSION_DENIED;
              //CHECK_INTERFACE(IBatteryPropertiesListener, data, reply);
              callbackFunction();
               return OK;
           case ON_CHANGED:
               if (!data.checkInterface(this))
                  return PERMISSION_DENIED;
               int nCode = data.readInt32() ;
               OnChanged(nCode) ;
               reply->writeInt32(OK) ;
               return OK ;
           }
          return BBinder::onTransact(code, data, reply, flags);
     }
     private:
     String16 mydescriptor;
};

int main()
{
     sp<IServiceManager> sm = defaultServiceManager();
     sp<IBinder> ibinder = sm->getService(String16(SAMPLE_SERIVCE_DES));
     if (ibinder == NULL){
          ALOGE("Client can't find Service");
           return -1;
     }
     Parcel _data,_reply;
     ALOGI("Client  find Service");
     SampeCallback *callback = new SampeCallback();
     status_t ret ;//= sm->addService(String16(SAMPLE_CB_SERIVCE_DES), callback);
    // ALOGI("Client addservice android.os.SampleCallback %d" , ret);
     _data.writeStrongBinder(sp<IBinder>(callback));
     ret = ibinder->transact(SRV_CODE, _data, &_reply, 0);
     ALOGI("Client transact binder android.os.SampleCallback %d" , ret);
     ProcessState::self()->startThreadPool();
//     IPCThreadState::self()->joinThreadPool();
     while(1){
         sleep(1) ;
         ALOGI("Service client alive");
     }
     return 0;
}

