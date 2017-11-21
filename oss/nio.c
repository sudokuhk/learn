//
//  nio.c
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#include "nio.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define HTTP            "http://"

static void setnoblock(int fd);
static int check_connect(int fd);
//static int closesock(int * pfd);
static int connect_with_timeo(int sock, struct sockaddr * addr, int timeo/*s*/);

void setnoblock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

int check_connect(int fd)
{
    //noblock
    char b;
    
    if (read(fd, (void *)&b, 1) < 0 && (errno == EAGAIN)) {
        return 1;
    } else {
        return 0;
    }
}

int closesock(int * pfd)
{
    printf("closesock\n");
    if (pfd != NULL && *pfd > 0) {
        close(*pfd);
    }
    *pfd = -1;
}

int connect_with_timeo(int sock, struct sockaddr * addr, int timeo/*s*/)
{
    fd_set rdset, wset;
    struct timeval to;
    int active = 0, error, len = sizeof(error);
    
    FD_ZERO(&rdset);
    FD_SET(sock, &rdset);
    wset = rdset;
    
    to.tv_sec = timeo;
    to.tv_usec = 0;
    
    if (connect(sock, addr, sizeof(*addr)) == 0) {
        printf("connect immediately\n");
        return 0;
    } else if (errno != EINPROGRESS) {
        printf("connect error!\n");
        return -1;
    }
    
    active = select(sock + 1, &rdset, &wset, NULL, &to);
    if (active < 0) {
        return -1;
    }
    
    if (FD_ISSET(sock, &rdset) || FD_ISSET(sock, &wset)) {
        if( getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            printf("connect error select\n");
            return -1;
        }
    }
    printf("connect succeed!\n");
    return 0;
}

int sockinit(int *pfd, char * url, const char * newurl)
{
    const char *phost, *pend;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    struct sockaddr addr;
    int res = 0;
    int sock = *pfd;
    time_t t;
    char host[100], port[10];
    
    printf("reinit, sock:%d\n", sock);
    
    printf("old:[%s], new:[%s]\n", url, newurl);
    if (strcmp(url, newurl) != 0) {
        printf("diff url, close\n");
        closesock(&sock);
    }
    
    if (sock > 0 && check_connect(sock) == 0) {
        closesock(&sock);
    }
    
    if (sock < 0) {
        printf("do connect!\n");
        strcpy(url, newurl);
        
        phost = url;
        if (strncmp(HTTP, url, sizeof(HTTP) - 1) == 0) {
            phost = url + sizeof(HTTP) - 1;
        }
        
        pend = strchr(phost, ':');
        if (pend != NULL) {
            strcpy(port, pend + 1);
            memcpy(host, phost, pend - phost);
            host[pend - phost] = '\0';
        } else {
            strcpy(host, phost);
            strcpy(port, "80");
        }
        
        printf("host[%s], port[%s]\n", host, port);
        
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family     = AF_INET;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype   = SOCK_STREAM; /* Datagram socket */
        hints.ai_flags      = 0;
        hints.ai_protocol   = 0;          /* Any protocol */
        
        t = time(NULL);
        res = getaddrinfo(host, port, &hints, &result);
        if (res != 0) {
            printf("getaddrinfo faile! %s\n", host);
            goto failed;
        }
        printf("dns resolver:%d\n", time(NULL) - t);
        
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            if (rp->ai_family == AF_INET) { //always true.
                addr = *rp->ai_addr;
                break;
            }
        }
        freeaddrinfo(result);
        
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            res = -1;
            sock = -1;
            goto failed;
        }
        
        setnoblock(sock);
        
        t = time(NULL);
        res = connect_with_timeo(sock, &addr, 10);
        if (res < 0) {
            closesock(&sock);
            goto failed;
        }
        printf("connect time:%d\n", time(NULL) - t);
    }
    
    res = 0;
failed:
    *pfd = sock;
    
    return res;
}

int writen(int *pfd, const char * data, int dsize)
{
    int nwrite = 0;
    int total  = 0;
    
    while (total < dsize) {
        nwrite = write(*pfd, data + total, dsize - total);
        if (nwrite < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                usleep(10/*ms*/);
                continue;
            }
            goto failed;
        }
        total += nwrite;
    }
    
    return dsize;
    
failed:
    closesock(pfd);
    return -1;
}

int readn(int *pfd, char * buf, int bsize, int timeo)
{
    fd_set rdset;
    struct timeval to;
    int total = 0, nread = 0;
    int sock = *pfd;
    
    FD_ZERO(&rdset);
    FD_SET(sock, &rdset);
    
    to.tv_sec = timeo;
    to.tv_usec = 0;
    
    if (select(sock + 1, &rdset, NULL, NULL, &to) < 0) {
        return 0; //timeout. thinks it's ok.
    }
    
    if (FD_ISSET(sock, &rdset)) {
        nread = read(sock, buf, bsize);
        if (nread < 0) {
            if (errno != EINTR) {  //socket error!
                printf("error:%d, %s\n", errno, strerror(errno));
                closesock(pfd);
                return -1;
            }
        } else if (nread == 0) {
            closesock(pfd);
            return -1;
        }
        return nread;
    }
    
    return 0;
}
