#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ConfigState.h"

extern void test_cpu_rate() ;
extern void test_cpu_rate_config();
extern void test_cpu_temp() ;
extern void test_cpu_temp_config()  ;

extern void test_mem_rate_config();
extern void test_mem_rate() ;

extern void test_date_config();
extern void test_date() ;
extern void test_runtime_config();

extern void test_battery() ;
extern void test_battery_config();

extern void test_audio_volume() ;
extern void test_audio_volume_config() ;

extern void test_wifi_config() ;



int main(int argc , const char* argv[]) {
    using namespace iris ;
    ConfigState::singleton()->start() ;

    test_cpu_rate() ;
    test_cpu_rate_config() ;
    test_cpu_temp() ;
    test_cpu_temp_config() ;

    test_mem_rate() ;
    test_mem_rate_config() ;

    test_date_config();
    test_date() ;
    test_runtime_config();

    test_battery() ;
    test_battery_config();

    test_audio_volume() ;
    test_audio_volume_config() ;

    test_wifi_config() ;

    while(true) {
        sleep(1) ;
    }

    return 0 ;
}
