#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//#include <platform/IMConfigState.h>
#include "ConfigState.h"
using namespace iris ;

std::string getDateTime() {
    time_t nowtime = time(0);
    char szDateTime[64] ;
    strftime(szDateTime, 63, "%F %A %X ", localtime(&nowtime));
    std::string strDateTime = szDateTime ;
    return strDateTime;
}

void test_date() {
   printf("Date Time %s \n" , getDateTime().c_str()) ;
}

void test_date_config() {

   printf("Date Time %s \n" , ConfigState::singleton()->getDate().c_str()) ;
}


void test_runtime_config() {

   printf("run Time %s \n" , ConfigState::singleton()->getSysRunTime().c_str()) ;
}