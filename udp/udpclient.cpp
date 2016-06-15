/**
 * \file
 * Brief description. Longer description.
 *
 * 
 * \author $Author$
 * \date $Date$
 */




#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include "udpclient.h"


extern bool udpDebugging;
static char hostName[1024];
static int port=13231;
static sockaddr_in servaddr;
static bool validaddr=false;

void initClient(const char *host,int p)
{
    strcpy(hostName,host);
    port = p;
    addrinfo *res;
    bzero(&servaddr,sizeof(servaddr));
    
    int rv = getaddrinfo(host,NULL,NULL,&res);
    if(rv){
        printf("Could not get host address %s: %s\n",
               host,gai_strerror(rv));
    } else {
        servaddr = *(sockaddr_in*)res->ai_addr;
        servaddr.sin_port = htons(port);
        validaddr=true;
    }
}

bool udpSend(const char *msg){
    if(!validaddr)return false;
    int fd = socket(AF_INET,SOCK_DGRAM,0);
    if(fd<0){
        perror("cannot open socket");
        return false;
    }
    
    if(udpDebugging)
        printf("SEND [%s(%x):%d] %s\n",hostName,servaddr.sin_addr.s_addr,
               port,msg);
    
    if (sendto(fd, msg, strlen(msg)+1, 0, // +1 to include terminator
               (sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("cannot send message");
        close(fd);
        return false;
    }
    close(fd);
    return true;
}
