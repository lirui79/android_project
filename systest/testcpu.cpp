#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//#include <platform/IMConfigState.h>
#include "ConfigState.h"
using namespace iris ;

long  mCPU[2][2];

int getCPUTime() {
    const char szFileName[] = "/proc/stat" ;
    FILE *fp = fopen(szFileName , "r") ;
    if (fp == NULL) {
       printf("getCPURaet() can not open file[%s]" , szFileName) ;
       return -1 ;
    }

    //for (int i = 0 ; i < 9 ; ++i) {
    char szCPU[16] = {0} ;
    long lCPU[10] = {0} ;
    fscanf(fp , "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n" , szCPU , &lCPU[0] , &lCPU[1] , &lCPU[2] , &lCPU[3] , &lCPU[4] , &lCPU[5] , &lCPU[6] , &lCPU[7] , &lCPU[8] , &lCPU[9]) ;
    printf("%s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n" , szCPU , lCPU[0] , lCPU[1] , lCPU[2] , lCPU[3] , lCPU[4] , lCPU[5] , lCPU[6] , lCPU[7] , lCPU[8] , lCPU[9]) ;
   // }
    for (int i = 1 ; i < 10 ; ++i)
       lCPU[0] += lCPU[i] ;
    mCPU[0][0] = mCPU[1][0] ;
    mCPU[0][1] = mCPU[1][1] ;

    mCPU[1][0] = lCPU[0] ;
    mCPU[1][1] = lCPU[3] ;

    fclose(fp) ;
    return 0 ;    
}

void test_cpu_rate() {
    for (int i = 0 ; i < 2 ; ++i) {
        for (int j = 0 ; j < 2 ; ++j)
          mCPU[i][j] = 0 ;
    }

    printf ("file[%s]:line[%d] CPU Rate[%d] \n" , __FILE__ , __LINE__ , getCPUTime()) ;
    sleep(1) ;
    printf ("file[%s]:line[%d] CPU Rate[%d] \n" , __FILE__ , __LINE__ , getCPUTime()) ;
    int nRate = 100 * ((mCPU[1][0] - mCPU[1][1]) - (mCPU[0][0] - mCPU[0][1])) / (mCPU[1][0] - mCPU[0][0]) ;
    printf("file[%s]:line[%d] CPU Rate[%d] \n" , __FILE__ , __LINE__ , nRate) ;
}

void test_cpu_rate_config() {
    int nRate = ConfigState::singleton()->getCPURate() ;
    printf("file[%s]:line[%d] CPU Rate[%d] \n" , __FILE__ , __LINE__ , nRate) ;
}

int getCPUTempPath(std::string &strTempPath , int &nCPUTRate) {
    const char szTempPath[] = "/sys/class/thermal/thermal_zone" ;
    const char *szCPUType[] = {"mtktscpu" , "tsens_tz_sensor" , "exynos" , "cpu_thermal_zone"} ;
    const int  nCPUTempRate[] = {1 , 1 , 1 , 1} ;
    for (int i = 0 ; i < 64 ; ++i) {
        char szTemp[64] ;
        sprintf(szTemp, "%s%d" , szTempPath , i) ;
        if (access(szTemp , F_OK | R_OK) < 0) {
            printf("file[%s]:line[%d] directory[%s] is not exist or read\n" , __FILE__ , __LINE__ , szTemp) ;
            break ;
        }

        char szTempType[64];
        sprintf(szTempType, "%s%d/type" , szTempPath , i) ;
        FILE *fp = fopen(szTempType , "r") ;
        if (fp == NULL) {
            printf("file[%s]:line[%d] directory[%s] is not open\n" , __FILE__ , __LINE__ , szTempType) ;
            continue ;
        }
        char szType[32] ;
        fscanf(fp , "%s" , szType) ;
        fclose(fp) ;
        int nFind = 0 ;
        for (int j = 0 ; j < 4 ; ++j) {
            if (strstr(szType , szCPUType[j]) == NULL)
              continue ;
            nFind = 1 ;
            nCPUTRate = nCPUTempRate[j] ;
            break ;
        }

        if (nFind == 0)
            continue ;
        strTempPath = szTemp;
        return 0 ;
    }

    return -1 ;
}

void test_cpu_temp() {
    std::string mCPUTemp = "";
    int nCPUTRate = 1 ;
    if (getCPUTempPath(mCPUTemp , nCPUTRate) < 0)
        return ;
    char szTempType[64] , szTempValue[64] ;
    sprintf(szTempType, "%s/type" ,  mCPUTemp.c_str()) ;
    sprintf(szTempValue, "%s/temp" , mCPUTemp.c_str()) ;
    FILE *fp = fopen(szTempType , "r") ;
    if (fp == NULL) {
        printf("file[%s]:line[%d] directory[%s] is not open\n" , __FILE__ , __LINE__ , szTempType) ;
        return ;
    }
    char szType[32] ;
    fscanf(fp , "%s" , szType) ;
    fclose(fp) ;
        
    fp = fopen(szTempValue , "r") ;
    if (fp == NULL) {
        printf("file[%s]:line[%d] directory[%s] is not open\n" , __FILE__ , __LINE__ , szTempValue) ;
        return ;
    }
    int nTemp ;
    fscanf(fp , "%d" , &nTemp) ;
    fclose(fp) ;
    nTemp /= nCPUTRate ;
    printf("file[%s]:line[%d] Temp type[%s][%d] \n" , __FILE__ , __LINE__ , szType , nTemp) ;
}


void test_cpu_temp_config() {
    int nRate = ConfigState::singleton()->getCPUTemperature() ;
    printf("file[%s]:line[%d] CPU Temperature[%d] \n" , __FILE__ , __LINE__ , nRate) ;
}
