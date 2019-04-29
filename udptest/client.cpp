#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



int nExit = 0;

void signal_handler(int sig)
{
	nExit = 1;
	printf("press <Ctrl-C> the program will end!") ;
}


// udpclient 192.168.1.53 8006 5000
int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: %s ip port\n", argv[0]);
        return -1;
    }


    struct sigaction sa;    /* Establish the signal handler. */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = ( void (*)(int) )signal_handler ;
    sigaction(SIGINT , &sa, NULL);

    int sock_id = -1;

    if ((sock_id = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket error\n");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    if (addr.sin_addr.s_addr == INADDR_NONE) {
        printf("ip address error\n");
        close(sock_id);
        return -2;
    }

    unsigned char data[28] = {0xFF};// 4 bytes len  8 bytes timestap ms  16bytes float
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0x18;
    struct timeval tv;
    unsigned char rdata[3096] = {0} ;

    float fArgv = 0.0;
    int num = 0;
    long timeArray[100] = {0};// 100ms
    int  frameArray[100] = {0};
    gettimeofday(&tv,NULL);
    long startTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    long min = 100000, max = -1;
    int moveWindow = 0;
    FILE *fp = fopen("/sdcard/upd_c.txt","w");
    int delay = atoi(argv[3]);
    fprintf(fp, "delay %d \n", delay);
    fprintf(fp, "current   max   min  argv\n");
    int size = 0;
    socklen_t szCAddr = sizeof(addr);

    while(nExit == 0) {
        usleep(delay);

          gettimeofday(&tv,NULL);
          long timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
          int  value = (timestamp >> 32) ;
          data[4] = ((value & 0xFF000000) >> 24);
          data[5] = ((value & 0x00FF0000) >> 16);
          data[6] = ((value & 0x0000FF00) >> 8);
          data[7] = ((value & 0x000000FF) >> 0);
          value = (timestamp & 0xFFFFFFFF) ;
          data[8] = ((value & 0xFF000000) >> 24);
          data[9] = ((value & 0x00FF0000) >> 16);
          data[10] =((value & 0x0000FF00) >> 8);
          data[11] =((value & 0x000000FF) >> 0);

        size = sendto(sock_id, data, 28, 0, (struct sockaddr*) &addr, sizeof(addr));

        if (size <= 0) {
            printf("sendto data wrong size %d\n", size);
            continue;
        }

        size = recvfrom(sock_id, rdata, 3096, 0, (struct sockaddr*) &addr, &szCAddr);
        if (size <= 0) {
            printf("recvfrom data wrong size %d\n", size);
            continue;
        }

        char address[32];
        inet_ntop(addr.sin_family, &addr.sin_addr, address, sizeof(address));
        printf("recffrom %s %d \n", address, size);

        gettimeofday(&tv,NULL);
          long timestamp2 = tv.tv_sec * 1000 + tv.tv_usec / 1000;
          int pos = (timestamp2 - startTime) / 100 ;
          moveWindow += pos;
          if (pos >= 100) {
              memset(timeArray, 0, sizeof(timeArray));
              memset(frameArray, 0 , sizeof(frameArray));
              pos = 0;
              startTime = timestamp2;
          } else {
              int i = 0 , j = pos ;

              if (i < j) {
                  for ( ; j < 99; ++i, ++j) {
                    timeArray[i] = timeArray[j];
                    frameArray[i] = frameArray[j];
                  }
                  for (; i < 99; ++i) {
                    timeArray[i] = 0;
                    frameArray[i] = 0;
                  }
                  startTime = timestamp2;
              }
          }

          value  = (data[4] << 24);
          value += (data[5] << 16);
          value += (data[6] << 8);
          value += (data[7]);
          long timestamp1 = value;
          timestamp1 = (timestamp1 << 32);
          value  = (data[8] << 24);
          value += (data[9] << 16);
          value += (data[10] << 8);
          value += (data[11]);
          timestamp1 = timestamp1 + value;
          timestamp2 = timestamp2 - timestamp1;

          timeArray[pos] += timestamp2;
          frameArray[pos] += 1;

          if (min > timestamp2)
              min = timestamp2;
          if (max < timestamp2)
              max = timestamp2;
          fArgv = 0.0;
          num = 0;
          for (int i = 0 ; i < 99 ; ++i) {
              fArgv += timeArray[i] ;
              num += frameArray[i] ;
          }

          printf("delay ms : %ld %ld %ld  %f\n", timestamp2, max, min, fArgv/num);
          fprintf(fp, "%ld %ld %ld  %f\n\n", timestamp2, max, min, fArgv/num);
          if (moveWindow < 100)
              continue ;
          max = -1;
          min = 1000000;
          moveWindow = 0;
    }

    fclose(fp);
    close(sock_id);

    return 0;
}

