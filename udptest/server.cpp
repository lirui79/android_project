
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



int nExit = 0;

void signal_handler(int sig)
{
	nExit = 1;
	printf("press <Ctrl-C> the program will end!") ;
}

// udpserver 8006
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s  port\n", argv[0]);
        return -1;
    }
    struct sigaction sa;    /* Establish the signal handler. */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = ( void (*)(int) )signal_handler ;
    sigaction(SIGINT , &sa, NULL);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int sock_id = -1;

    if ((sock_id = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket error\n");
        return -1;
    }

    if (bind(sock_id, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("bind error\n");
        close(sock_id);
        return -2;
    }

    unsigned char data[28] = {0};
    unsigned char sdata[3096] = {0} ;
    sdata[0] = 0;
    sdata[1] = 0;
    sdata[2] = 0x0C ;
    sdata[3] = 0x18 ;
    for (int i = 4 ; i < 3096; ++i) {
        sdata[i] = (i % 255);
    }
    struct sockaddr_in  clientAddr;
    int size = 0;
    socklen_t szCAddr = sizeof(clientAddr);

    while(nExit == 0) {
        size = recvfrom(sock_id, data, 28, 0, (struct sockaddr*) &clientAddr, &szCAddr);
        if (size <= 0) {
            printf("recvfrom data wrong size %d\n", size);
            continue;
        }

        char clientAddress[32];
        inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, clientAddress, sizeof(clientAddress));
        printf("recffrom %s %d \n", clientAddress, size);
        memcpy(sdata + 4, data + 4 , 8);

        size = sendto(sock_id, sdata, 3096, 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));

        if (size <= 0) {
            printf("sendto data wrong size %d\n", size);
            continue;
        }

    }

    close(sock_id);

    return 0;
}
