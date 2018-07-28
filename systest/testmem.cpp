#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//#include <platform/IMConfigState.h>
#include "ConfigState.h"
using namespace iris ;

int getMEMRate() {
    const char szFileName[] = "/proc/meminfo" ;
    FILE *fp = fopen(szFileName , "r") ;
    if (fp == NULL) {
       printf("getMEMRate() can not open file[%s]" , szFileName) ;
       return -1 ;
    }

    while(!feof(fp)) {
       char szMemType[32] , szMemUnit[32] ;
       long lMemSize = 0 ;
       fscanf(fp , "%s %ld %s\n" , szMemType , &lMemSize , szMemUnit) ;
       printf("%s %ld %s\n" , szMemType , lMemSize , szMemUnit) ;
    }
    fclose(fp) ;

    return 0 ;
}

void test_mem_rate() {
    //getMEMRate() ;
    const char szFileName[] = "/proc/meminfo" ;
    FILE *fp = fopen(szFileName , "r") ;
    if (fp == NULL) {
        printf("getMEMRate() can not open file[%s]" , szFileName) ;
        return ;
    }

    std::string strMemInfo;
    int nCount = 0 ;
    while(!feof(fp) && nCount < 2) {
        char szMemType[32] , szMemSize[32] , szMemUnit[32] ;
        fscanf(fp , "%s %s %s\n" , szMemType , szMemSize , szMemUnit) ;
        strMemInfo += szMemType ;
        strMemInfo += " ";
        strMemInfo += szMemSize ;
        strMemInfo += " ";
        strMemInfo += szMemUnit ;
        if (nCount == 0) 
            strMemInfo += " - ";

        printf("%s %s %s\n" , szMemType , szMemSize , szMemUnit) ;
        ++nCount ;
    }
    fclose(fp) ;

    printf("%s \n" , strMemInfo.c_str()) ;
}

void test_mem_rate_config() {
    double fMRate = 1.0 ;
    std::string strMem = ConfigState::singleton()->getMEMInfo(&fMRate) ;
    printf("file[%s]:line[%d] meminfo[%s]  %f \n" , __FILE__ , __LINE__ , strMem.c_str() , fMRate) ;
}