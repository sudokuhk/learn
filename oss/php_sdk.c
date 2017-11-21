//
//  php_sdk.c
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#include "php_sdk.h"
#include "nio.h"
#include "http_parse.h"

#include <stdio.h>

static int  php_sock = -1;
static char php_url[1024] = {0};

int do_php_post_request(const char * host, const char * path, const char * type,
    const char * data, size_t ndata, array * arr)
{
    char content[2048], date[50];
    int off, woff, roff, nread, ndeal, nstatus;
    int ret = -1;
    const int max_ibuf = 2048;
    char * version, *status, *reason, *header;
    const char *next, *phost;
    int nlength = 0, remain, total;

    nowtime(date);
    if (type == NULL) {
        type = DEF_TYPE;
    }
    
    phost = host;
    if (strncmp(HTTP, phost, sizeof(HTTP) - 1) == 0) {
        phost += sizeof(HTTP) - 1;
    }
    
    off = 0;
    off += sprintf(content + off, "POST %s "HTTP_V1 NEWLINE, path);
    off += sprintf(content + off, "Host: %s"NEWLINE, phost);
    off += sprintf(content + off, CONTENT_LENGTH"%d"NEWLINE, ndata);
    off += sprintf(content + off, DATE"%s"NEWLINE, date);
    off += sprintf(content + off, KEEP_ALIVE NEWLINE);
    off += sprintf(content + off, CONTENT_TYPE"%s"NEWLINE, type);
    off += sprintf(content + off, ACCEPT NEWLINE);
    off += sprintf(content + off, AGENT NEWLINE);
    off += sprintf(content + off, NEWLINE);
    
    memcpy(content + off, data, ndata);
    off += ndata;
    content[off] = '\0';
    
    printf("content:%s\n", content);
    
    if (sockinit(&php_sock, php_url, host) < 0) {
        goto error;
    }
    
    writen(&php_sock, content, off);
    
    woff = roff = 0;
response:
    nread = readn(&php_sock, content, max_ibuf, 10);
    if (nread < 0) {
        goto error;
    } else if (nread == 0) {
        goto response;
    }
    
    woff = nread;
    
    //parse http.
startline:
    ndeal = http_startline(content + roff, woff - roff,
                           &version, &status, &reason);
    if (ndeal == 0) {
        nread = readn(&php_sock, content + woff, max_ibuf - woff, 10);
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
    
headerlable:
    ndeal = http_headers(content + roff, woff - roff, &header);
    if (ndeal == 0) {
        nread = readn(&php_sock, content + woff, max_ibuf - woff, 10);
        if (nread < 0) {
            goto error;
        }
        goto headerlable;
    }
    roff += ndeal;

    next = strstr(header, CONTENT_LENGTH);
    if (next == NULL) { //no content-length info, no bottom data.
        goto out;
    }
    next = http_header_content(next);
    nlength = atoi(next);
    
    printf("content-length:%d\n", nlength);
    if (nlength == 0) {
        goto out;
    }
    
    remain = woff - roff;
    printf("remain:%d\n", remain);
    
    total = nlength;
    nlength -= remain;
    push_array(arr, content + roff, remain);
    
    while (nlength > 0) {
        nread = readn(&php_sock, content, max_ibuf, 10);
        if (nread < 0) {
            goto error;
        } else if (nread > 0) {
            nlength -= nread;
            push_array(arr, content, nread);
        }
    }
    
out:
    if (nstatus == 200) {
        ret = 0;
    }
error:
    return ret;
}
