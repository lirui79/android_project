
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
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
    int length = size  ,  sz;
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


int main(int argc, char *argv[])
{
       int server_sockfd;//服务器端套接字
       int client_sockfd;//客户端套接字

       int len;
       struct sockaddr_in my_addr;   //服务器网络地址结构体
       struct sockaddr_in remote_addr; //客户端网络地址结构体

       socklen_t sin_size;
       char buf[BUFSIZ];  //数据传送的缓冲区
       memset(&my_addr,0,sizeof(my_addr)); //数据初始化--清零
       my_addr.sin_family=AF_INET; //设置为IP通信
       my_addr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
       my_addr.sin_port=htons(8000); //服务器端口号

       /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
       if((server_sockfd = socket(PF_INET,SOCK_STREAM,0))<0) {
              perror("socket");
              return 1;
       }

        /*将套接字绑定到服务器的网络地址上*/
       if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)  {
              perror("bind");
              return 1;
       }

       /*监听连接请求--监听队列长度为5*/
       listen(server_sockfd,5);

       sin_size=sizeof(struct sockaddr_in);



       /*等待客户端连接请求到达*/

       if((client_sockfd = accept(server_sockfd,(struct sockaddr *)&remote_addr,&sin_size))<0) {

              perror("accept");

              return 1;

       }

       printf("accept client %s\n",inet_ntoa(remote_addr.sin_addr));

     //  len=send(client_sockfd,"Welcome to my server\n",21,0);//发送欢迎信息
     int nZero=0;
//     setsockopt(client_sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&nZero,sizeof(int));
//     setsockopt(client_sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&nZero,sizeof(int));


       /*接收客户端的数据并将其发送给客户端--recv返回接收到的字节数，send返回发送的字节数*/
    unsigned char data[28] = {0};
    unsigned char sdata[3096] = {0} ;
    sdata[0] = 0;
    sdata[1] = 0;
    sdata[2] = 0x0C ;
    sdata[3] = 0x18 ;
    while(1) {

        if (read(client_sockfd, data, 28) < 28) {
            printf("server receive data from server wrong, exit\n");
            break;
        }

        memcpy(sdata + 4, data + 4 , 8);

        if (write(client_sockfd, sdata, 3096) < 3096) {
            printf("server send data to client wrong, exit\n");
        }
    }

    close(client_sockfd);
    close(server_sockfd);
    return 0;
}
