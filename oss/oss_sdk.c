//
//  oss_sdk.c
//  Learn
//
//  Created by huangkun on 3/18/17.
//
//

#include "oss_sdk.h"
#include "http_parse.h"
#include "hmac_sha1.h"
#include "nio.h"
#include "base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MACRO_LEN(mac)  (sizeof(mac) - 1)

#define MAX_BSIZE   (16 * 1024)

const int   max_bsize = MAX_BSIZE; //16k
static char rbuf[MAX_BSIZE] = {0};
static char url[1024] = {0};
static int  oss_sock = -1;

static void calc_auth(char * out, const char * secret,
    const char * method, const char * md5, const char * type,
    const char * date, const char * bucket, const char * path)
{
    int off = 0;
    
    char all[1024];
    char hmac[20];

    off += sprintf(all + off, "%s\n", method);

    if (md5 != NULL) {
        off += sprintf(all + off, "%s\n", md5);
    } else {
        off += sprintf(all + off, "\n");
    }
    
    if (type != NULL) {
        off += sprintf(all + off, "%s\n", type);
    } else {
        off += sprintf(all + off, "\n");
    }
    
    off += sprintf(all + off, "%s\n", date);
    off += sprintf(all + off, "/%s/%s", bucket, path);
    
    //printf("auth:\n[%s]\n", all);
    hmac_sha1(hmac, secret, strlen(secret), all, off);
    base64_encode(hmac, out, 20);
}

int oss_sdk_upload(ccp endpoint, ccp bucket, ccp key, ccp secret,
                   ccp remote, ccp local, ccp type, oss_sdk_callback cb)
{
    char content[1024];
    char date[50];
    char buf[100], newurl[100];
    
    const char * p, *next;
    char *version, *status, *reason, *header;
    
    int fd = -1, ret = -1;
    int ndata = 0, nread, nwrite, ndeal, dlength, remain, total, nstatus;
    struct stat sb;
    int  roff = 0, woff = 0, soff = 0;
    
    nowtime(date);
    
    p = endpoint;
    if (strncmp(HTTP, endpoint, sizeof(HTTP) - 1) == 0) {
        p += sizeof(HTTP) - 1;
    }
    
    if (type == NULL) {
        type = DEF_TYPE;
    }
    
    fd = open(local, O_RDONLY);
    if (fd < 0) {
        goto error;
    }
    if(fstat(fd, &sb) < 0) {
        goto error;
    }
    ndata = sb.st_size;
    
    sprintf(newurl, "%s.%s", bucket, p);
    calc_auth(buf, secret, "PUT", NULL, type, date, bucket, remote);
    
    soff = 0;
    soff += sprintf(content + soff, "PUT /%s HTTP/1.1\r\n", remote);
    soff += sprintf(content + soff, "Host: %s.%s\r\n", bucket, p);
    soff += sprintf(content + soff, "Date: %s\r\n", date);
    soff += sprintf(content + soff, "Content-Length: %d\r\n", ndata);
    soff += sprintf(content + soff, "Authorization: OSS %s:%s\r\n", key, buf);
    soff += sprintf(content + soff, "Content-Type: %s\r\n", type);
    soff += sprintf(content + soff, "%s\r\n%s\r\n%s\r\n",
                    AGENT, ACCEPT, KEEP_ALIVE);
    soff += sprintf(content + soff, "\r\n");

    //printf("%s", content);
    
    sockinit(&oss_sock, url, newurl);
    writen(&oss_sock, content, soff);
 
    while ((nread = read(fd, rbuf, max_bsize)) > 0) {
        if (writen(&oss_sock, rbuf, nread) != nread) {
            goto error;
        }
    }
    
    //read response.
    woff = roff = 0;
response:
    nread = readn(&oss_sock, rbuf, max_bsize, 10);
    if (nread < 0) {
        goto error;
    } else if (nread == 0) {
        goto response;
    }
    
    woff = nread;
    
    //parse http.
startline:
    ndeal = http_startline(rbuf + roff, woff - roff,
                           &version, &status, &reason);
    if (ndeal == 0) {
        nread = readn(&oss_sock, rbuf + woff, max_bsize - woff, 10);
        if (nread < 0) {
            goto error;
        }
        goto startline;
    } else if (ndeal < 0) {
        goto error;
    }
    roff += ndeal;
    nstatus = atoi(status);
    
    printf("ndeal:%d\n", ndeal);
    printf("status:%d\n", nstatus);
    //printf("[%s]\n", rbuf + roff);
    
headerlable:
    ndeal = http_headers(rbuf + roff, woff - roff, &header);
    if (ndeal == 0) {
        nread = readn(&oss_sock, rbuf + woff, max_bsize - woff, 10);
        if (nread < 0) {
            goto error;
        }
        goto headerlable;
    }
    roff += ndeal;
    printf("ndeal:%d\n", ndeal);
    
    next = strstr(header, "Content-Length:");
    if (next == NULL) { //no content-length info, no bottom data.
        goto out;
    }
    next = http_header_content(next);
    dlength = atoi(next);
    
    printf("content-length:%d\n", dlength);
    if (dlength == 0) {
        goto out;
    }
    
    remain = woff - roff;
    printf("remain:%d\n", remain);

    total = dlength;
    dlength -= remain;
    while (dlength > 0) {
        nread = readn(&oss_sock, rbuf, max_bsize, 10);
        dlength -= nread;
    }
  
out:
    if (nstatus == 200) {
        ret = 0;
    }
    
error:
    if (fd > 0) {
        close(fd);
    }
    if (ret == 0) {
        printf("upload succeed!\n");
    } else {
        printf("upload failed!\n");
    }
    return ret;
}


int oss_sdk_download(ccp endpoint, ccp bucket, ccp key, ccp secret,
                     ccp remote, ccp local, oss_sdk_callback cb)
{
    char content[1024];
    char date[50];
    char buf[100], newurl[100];
    
    const char * p;
    int ret = -1;
    int nread, ndeal;
    char * version, * status, * reason;
    char * next;
    char * header;
    int dlength = 0, total = 0;
    int remain = 0;
    int nstatus = 0;
    int fd = -1;
    int  roff = 0, woff = 0, soff = 0;
    
    nowtime(date);

    p = endpoint;
    if (strncmp(HTTP, endpoint, sizeof(HTTP) - 1) == 0) {
        p += sizeof(HTTP) - 1;
    }
    
    sprintf(newurl, "%s.%s", bucket, p);
    calc_auth(buf, secret, "GET", NULL, NULL, date, bucket, remote);
    
    soff = 0;
    soff += sprintf(content + soff, "GET /%s HTTP/1.1\r\n", remote);
    soff += sprintf(content + soff, "Host: %s.%s\r\n", bucket, p);
    soff += sprintf(content + soff, "Date: %s\r\n", date);
    soff += sprintf(content + soff, "Authorization: OSS %s:%s\r\n", key, buf);
    soff += sprintf(content + soff, "%s\r\n%s\r\n%s\r\n",
                    AGENT, ACCEPT, KEEP_ALIVE);
    soff += sprintf(content + soff, "\r\n");
    
    sockinit(&oss_sock, url, newurl);
    writen(&oss_sock, content, soff);

    woff = roff = 0;
    woff = readn(&oss_sock, rbuf, max_bsize, 10);
    if (woff < 0) {
        goto error;
    }
    printf("woff:%d\n", woff);
    
    //parse http.
startline:
    ndeal = http_startline(rbuf + roff, woff - roff,
                           &version, &status, &reason);
    if (ndeal == 0) {
        nread = readn(&oss_sock, rbuf + woff, max_bsize - woff, 10);
        if (nread < 0) {
            goto error;
        }
        goto startline;
    } else if (ndeal < 0) {
        goto error;
    }
    roff += ndeal;
    nstatus = atoi(status);
    
    printf("ndeal:%d\n", ndeal);
    printf("status:%d\n", nstatus);
    //printf("[%s]\n", rbuf + roff);
    
headerlable:
    ndeal = http_headers(rbuf + roff, woff - roff, &header);
    if (ndeal == 0) {
        nread = readn(&oss_sock, rbuf + woff, max_bsize - woff, 10);
        if (nread < 0) {
            goto error;
        }
        goto headerlable;
    }
    roff += ndeal;
    printf("ndeal:%d\n", ndeal);
    
    next = strstr(header, "Content-Length:");
    if (next == NULL) { //no content-length info, no bottom data.
        goto out;
    }
    next += strlen("Content-Length:");
    while (*next == ' ') next ++;
    dlength = atoi(next);
    printf("content-length:%d\n", dlength);
    if (dlength == 0) {
        goto out;
    }
    
    remain = woff - roff;
    printf("remain:%d\n", remain);
    fd = open(local, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        goto error;
    }
    
    if (nstatus == 200) {
        write(fd, rbuf + roff, remain);
    }
    
    total = dlength;
    dlength -= remain;
    while (dlength > 0) {
        nread = readn(&oss_sock, rbuf, max_bsize, 10);
        dlength -= nread;
        
        //printf("total:%d, read:%d, read:%d\n", total, total - dlength, nread);
        if (nstatus == 200) {
            write(fd, rbuf, nread);
        }
    }

out:
    if (nstatus == 200) {
        ret = 0;
    }
    
error:
    if (fd > 0) {
        close(fd);
    }
    if (ret == 0) {
        printf("download succeed!\n");
    } else {
        printf("download failed!\n");
    }
    return ret;
}
