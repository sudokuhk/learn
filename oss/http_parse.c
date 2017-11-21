//
//  http_parse.c
//  Learn
//
//  Created by huangkun on 3/18/17.
//
//

#include "http_parse.h"

#include <string.h>
#include <time.h>

#define MACRO(m)    (sizeof(m) - 1)

#define JUDGE(pch, target)    ((*(pch)) == (target))

static char * skip_ws(char *p)
{
    while (*p == ' ') {
        p ++;
    }
    return p;
}

void nowtime(char date[50])
{
    time_t t_time = time(NULL);
    struct tm tm_time;
    
    gmtime_r(&t_time, &tm_time);
    strftime(date, 50, "%a, %d %b %Y %H:%M:%S %Z", &tm_time);
}

/*
 * return consume how many bytes in buf.
 * = 0:  not find \r\n, but it's http response.
 * > 0:  find
 * < 0:  not http response format.
 */
int http_startline(char * buf, int bsize,
                       char ** version, char ** code, char ** reason)
{
    int ncmp = 0, nconsume = 0, nsize = 0;
    char * pf;
    
    ncmp = MACRO(HTTP_V1) > bsize ? bsize : MACRO(HTTP_V1);
    if (strncmp(HTTP_V0, buf, ncmp) != 0 &&
        strncmp(HTTP_V1, buf, ncmp) != 0) {
        return -1;
    }
    
    pf = (char *)memchr(buf, '\r', bsize - 1);
    if (pf == NULL) {
        return 0;
    }
    
    if (!JUDGE(pf + 1, '\n')) { //first line should not appear '\r' alone.
        return -1;
    }
    
    nconsume = (pf - buf + 1) + 1/*\n*/;
    
    //http version
    *version = buf;
    
    //http result code
    pf = (char *)memchr(buf, ' ', bsize);
    if (pf == NULL) {
        return -1;
    }
    pf = skip_ws(pf);
    *code = pf;
    
    //http result reason
    pf = (char *)memchr(pf, ' ', bsize - (buf - pf));
    if (pf == NULL) {
        return -1;
    }
    pf = skip_ws(pf);
    *reason = pf;
    
    return nconsume;
}

int http_headers(char * buf, int bsize, char ** header)
{
    char * p, *s;
    int nconsume = 0;
    s = buf;
    * header = buf;
    
again:
    nconsume = s - buf;
    p = (char *)memchr(s, '\r', bsize - nconsume);
    
    nconsume = p - buf;
    if (nconsume < 3) { //not find \r\n\r\n
        return 0;
    }
    
    if (JUDGE(p + 1, '\n') && JUDGE(p + 2, '\r') && JUDGE(p + 3, '\n')) {//find
        nconsume = p - buf + 1 + 3;
        return nconsume;
    }
    
    s = p + 2;
    goto again;
}

const char * http_header_content(const char * header)
{
    const char * p;
    p = strchr(header, ':');
    while (* (++ p) == ' ');

    return p;
}
