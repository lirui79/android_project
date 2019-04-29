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


int write(int sckid, const unsigned char *data, int size) {
    const unsigned char *sdata = data ;
    int length = size ;
    if (sckid < 0)
        return -1 ;
    while(length > 0) {
        int sz = send(sckid, sdata, length, MSG_NOSIGNAL|MSG_WAITALL) ;
		if (sz <= 0) {
            if (errno == EINTR)
                sz = 0 ;
            else if (errno == EAGAIN)
                sz = 0 ;
            else
				return -2 ;
		}
        length -= sz ;
        sdata += sz ;
      }
     return (size - length) ;
}

int read(int sckid, unsigned  char *data , int size) {
    unsigned char *dest = data ;
    int length = size, sz;
    if (sckid < 0)
         return -1 ;

    while(length > 0) {
        sz = recv(sckid, dest, length, MSG_NOSIGNAL|MSG_WAITALL) ;
        if (sz <= 0) {
            if (errno == EINTR)
                sz = 0 ;
            else if (errno == EAGAIN)
                sz = 0 ;
            else
                return -2 ;
        }
        length -= sz ;
        dest += sz ;
    }
    return (size - length) ;
}

int nExit = 0;

void signal_handler(int sig)
{
	nExit = 1;
	printf("press <Ctrl-C> the program will end!") ;
}


// tcpclient 10.0.0.4 5000
int main(int argc, char *argv[]) {
    struct sigaction sa;    /* Establish the signal handler. */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = ( void (*)(int) )signal_handler ;
    sigaction(SIGINT , &sa, NULL);
       int client_sockfd;

       int len;

       struct sockaddr_in remote_addr; //服务器端网络地址结构体

     //  char buf[BUFSIZ];  //数据传送的缓冲区

       memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零

       remote_addr.sin_family=AF_INET; //设置为IP通信

       remote_addr.sin_addr.s_addr=inet_addr(argv[1]);//服务器IP地址

       remote_addr.sin_port=htons(8000); //服务器端口号

       int delay = atoi(argv[2]);

       /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/

       if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)  {

              perror("socket");

              return 1;

       }



       /*将套接字绑定到服务器的网络地址上*/

       if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)   {

              perror("connect");

              return 1;

       }
        printf("connected to server\n");
       int nZero=0;
      // setsockopt(client_sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&nZero,sizeof(int));
      // setsockopt(client_sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&nZero,sizeof(int));
    //   len = recv(client_sockfd,buf,BUFSIZ,0);//接收服务器端信息

    //     buf[len]='\0';

    //   printf("%s",buf); //打印服务器端信息



       /*循环的发送接收信息并打印接收信息--recv返回接收到的字节数，send返回发送的字节数*/
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
       FILE *fp = fopen("/sdcard/tcp_c.txt","w");
       fprintf(fp, "delay %d \n", delay);
       fprintf(fp, "current   max   min  argv\n");
       while(nExit == 0)  {
          usleep(delay);
          //printf("Enter string to send:");

          //scanf("%s",buf);

        // if(!strcmp(buf,"quit"))
        //         break;
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

          if (write(client_sockfd, data, 28) < 28) {
              printf("send to server wrong, exit\n");
              break;
          }

          if (read(client_sockfd,rdata,3096) < 3096) {
              printf("receive data from server wrong, exit\n");
              break;
          }

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

    close(client_sockfd);//关闭套接字
    fclose(fp);
    return 0;

}
