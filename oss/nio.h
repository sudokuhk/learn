//
//  nio.h
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#ifndef __NIO_H__
#define __NIO_H__

//
//  nio.c
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#include "nio.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

int closesock(int * pfd);

int sockinit(int *pfd, char * url, const char * newurl);

int writen(int *pfd, const char * data, int dsize);

int readn(int *pfd, char * buf, int bsize, int timeo);

#endif /* __NIO_H__ */
