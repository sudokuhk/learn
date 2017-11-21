/*************************************************************************
    > File Name: mydownload.c
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thu 16 Mar 2017 05:22:31 PM CST
 ************************************************************************/

#include<stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

size_t http_write_callback(void * ptr, size_t size, size_t nmemb, void * userp);
size_t http_read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

typedef struct http_response
{
    int size;
    int capacity;

    void * data;
} http_response;



http_response * get_http_response(int capacity)
{
    http_response * response = NULL;

    response = (http_response *)malloc(sizeof(http_response));
    if (response != NULL) {
        response->size = 0;
        response->capacity = 0;
        response->data = NULL;
        
        if (capacity > 0) {
            expand_response(response, capacity);
        }
    }
    
    return response;
}

void put_http_response(struct http_response ** res)
{
    struct http_response * p = NULL;
    
    if (res == NULL) {
        return;
    }
    
    p = *res;
    
    if (p != NULL) {
        if (p->capacity > 0) {
            free(p->data);
        }
        free(p);
        p = NULL;
    }
}

int http_post(const char * url, int timeo, http_response * res,
              const char * data, int dsize, const char * mime)
{
    CURL *curl = NULL;
    CURLcode curlres;
    struct curl_slist * plist = NULL;
    
    int ret = 0;
    
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeo);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    
    //printf("url:%s\n", url);
    //printf("data:%d:%s\n", dsize, data);
    if (dsize > 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, dsize);
    }

    if (mime != NULL) {
        plist = curl_slist_append(NULL, mime);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
    }
    curlres = curl_easy_perform(curl);
    if(curlres != CURLE_OK) {
        ret = -1;
    }
    if (plist != NULL) {
        curl_slist_free_all(plist);
        plist = NULL;
    }
    curl_easy_cleanup(curl);
    
    return ret;
}

int http_get(const char * url, int timeo, http_response * res)
{
    CURL *curl = NULL;
    CURLcode curlres;
    int ret = 0;
    
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeo);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res);
    
    curlres = curl_easy_perform(curl);
    printf("curl res:%d\n", curlres);
    if(curlres != CURLE_OK) {
        ret = -1;
    }
    curl_easy_cleanup(curl);
    
    return ret;
}

int http_put(const char * url, int timeo, http_response * res, int fd, int fsize,
             struct curl_slist * headers)
{
    CURL *curl = NULL;
    CURLcode curlres;
    int ret = 0;
    
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeo);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, http_read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&fd);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    curlres = curl_easy_perform(curl);
    printf("curl res:%d\n", curlres);
    if(curlres != CURLE_OK) {
        ret = -1;
    }
    curl_easy_cleanup(curl);
    
    return ret;
}

int expand_response(struct http_response * res, int capacity)
{
    char * newp = NULL;
    int ret = 0;
    
    if (res->data == NULL) {
        newp = (void *)malloc(capacity);

        if (newp == NULL) {
            ret = -1;
        } else {
            res->size     = 0;
            res->capacity = capacity;
            res->data     = newp;
            *((char *)res->data) = 0; //init
        }
    } else {
        newp = (void *)realloc(res->data, capacity);

        if (newp != NULL) {
            if (res->size > capacity) {
                res->size = capacity;
            }
            res->capacity = capacity;
            res->data = newp;
        } else {
            ret = -1;
        }
    }

    return ret;
}

size_t http_read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t nread = 0;
    int fd = *(int *)stream;
    
    printf("readcallback, size:%d, nmemb:%d\n", size, nmemb);
    nread = read(fd, ptr, size * nmemb);

    return nread;
}

size_t http_write_callback(void * ptr, size_t size,
    size_t nmemb, void * userp)
{
    struct http_response * pres = NULL;
    int remain  = 0;
    int recvlen = size * nmemb;
    
    pres = (struct http_response *)userp;
    if (pres == NULL) {
        goto error;
    }
    
    remain = pres->capacity - pres->size;
    if (remain < recvlen) {
        if (expand_response(pres, pres->capacity + recvlen + 1)) {
            goto error;
        }
    }
    
    memcpy(((char *)pres->data + pres->size), (char *)ptr, recvlen);
    pres->size += recvlen;
    *((char *)pres->data + pres->size) = 0;
error:
    return recvlen;
}

static char escape_hash[128];

void init()
{
    int i;
    
    const static char http_url_escape[] = {
        ' ', '"', '#', '%', '&', '(', ')',
        '+', ',', '/', ':', ';', '<',
        '=', '>', '?', '@', '\\', '|',
    };
    
    memset((void *)escape_hash, 0x00,
           sizeof(escape_hash) * sizeof(escape_hash[0]));
    
    for (i = 0; i < sizeof(http_url_escape) / sizeof(http_url_escape[0]); i++) {
        escape_hash[http_url_escape[i]] = 1;
    }
}

int oss_download(const char * endpoint, const char * bucket,
                 const char * key, const char * secret,
                 const char * remote, const char * local)
{
    /*
     GET /test%2Fhuangkun%2F188_F775b66f03c37967859B%2F188_F775b66f03c37967859B.m3u8 HTTP/1.1\r\n
     Host: pingqu.oss-cn-shenzhen.aliyuncs.com\r\n
     User-Agent: aliyun-sdk-c/3.4.0(Compatible Unknown)\r\n
     Accept: *\/*\r\n
     Date: Thu, 16 Mar 2017 09:07:35 GMT\r\n
     Authorization: OSS AOfJGJvJfgcy1Onu:ro5TBs9yd9hqIuyQoBJH8961MTQ=\r\n
     \r\n
    [Full request URI: http://pingqu.oss-cn-shenzhen.aliyuncs.com/test%2Fhuangkun%2F188_F775b66f03c37967859B%2F188_F775b66f03c37967859B.m3u8]
                   url=http://pingqu.oss-cn-shenzhen.aliyuncs.com/test/huangkun/188_F775b66f03c37967859B/188_F775b66f03c37967859B.m3u8

     */
    char url[1024];
    const int url_max = 1024;
    const char * pendpoint = endpoint;
    static const char http[] = "http://";
    static const int nhttp = sizeof(http) - 1;
    int nsize = 0;
    http_response * httpres;
    
    if (strncmp(http, endpoint, nhttp) == 0) {
        pendpoint += nhttp;
    }
    
    nsize = snprintf(url, url_max, "http://%s.%s/%s", bucket, pendpoint, remote);
    printf("url=%s\n", url);
    
    httpres = get_http_response(2048);
    if (httpres == NULL) {
        goto error;
    }
    
    if (http_get((const char *)url, 10, httpres) < 0){
        goto error;
    }
    
    //printf("data:%s\n", (const char *)httpres->data);
error:
    return 0;
}

int oss_upload(const char * endpoint, const char * bucket,
                 const char * key, const char * secret,
                 const char * remote, const char * local)
{
    /*
     GET /test%2Fhuangkun%2F188_F775b66f03c37967859B%2F188_F775b66f03c37967859B.m3u8 HTTP/1.1\r\n
     Host: pingqu.oss-cn-shenzhen.aliyuncs.com\r\n
     User-Agent: aliyun-sdk-c/3.4.0(Compatible Unknown)\r\n
     Accept: *\/*\r\n
     Date: Thu, 16 Mar 2017 09:07:35 GMT\r\n
     Authorization: OSS AOfJGJvJfgcy1Onu:ro5TBs9yd9hqIuyQoBJH8961MTQ=\r\n
     \r\n
     [Full request URI: http://pingqu.oss-cn-shenzhen.aliyuncs.com/test%2Fhuangkun%2F188_F775b66f03c37967859B%2F188_F775b66f03c37967859B.m3u8]
     url=http://pingqu.oss-cn-shenzhen.aliyuncs.com/test/huangkun/188_F775b66f03c37967859B/188_F775b66f03c37967859B.m3u8
     
     */
    char url[1024];
    char auth[1024];
    const int url_max = 1024;
    const char * pendpoint = endpoint;
    static const char http[] = "http://";
    static const int nhttp = sizeof(http) - 1;
    int nsize = 0;
    http_response * httpres;
    int fd;
    struct stat sb;
    struct curl_slist * plist = NULL;
    char buffer[256] = { 0 };
    char date[100];
    time_t t_time = time(NULL);
    struct tm tm_time;
    char oss[1024], hmac[20], base64[100];
    
    if (strncmp(http, endpoint, nhttp) == 0) {
        pendpoint += nhttp;
    }

    gmtime_r(&t_time, &tm_time);
    strftime(buffer, sizeof(buffer),
             "%a, %d %b %Y %H:%M:%S %Z", &tm_time);
    snprintf(date, 100, "Date: %s", (const char *)buffer);
    
    // method md5 content-type date path
    nsize = snprintf(oss, 1024, "PUT\n\n%s\n%s\n/%s/%s",
                     "application/vnd.apple.mpegurl", "Mon, 20 Mar 2017 04:23:53 GMT"
                     , bucket, remote);
    hmac_sha1(hmac, secret, strlen(secret), oss, nsize);
    base64_encode(hmac, base64, 20);
    printf("oss[%s]\n, base64:%s\n", oss, base64);
    return 0;

    nsize = snprintf(url, url_max, "http://%s.%s/%s", bucket, pendpoint, remote);
    printf("url=%s\n", url);
    snprintf(auth, 1024, "Authorization: OSS %s:%s", key, base64);
    
    plist = curl_slist_append(plist, (const char *)auth);
    plist = curl_slist_append(plist, (const char *)date);
    plist = curl_slist_append(plist, (const char *)"Content-Type: application/vnd.apple.mpegurl");
    
    httpres = get_http_response(2048);
    if (httpres == NULL) {
        goto error;
    }
    
    fd = open(local, O_RDONLY);
    fstat(fd, &sb);
    
    if (http_put((const char *)url, 10, httpres, fd, sb.st_size, plist) < 0){
        goto error;
    }
    printf("res:%s\n", (const char *)httpres->data);
    close(fd);
    
    if (plist != NULL) {
        curl_slist_free_all(plist);
        plist = NULL;
    }
    
    //printf("data:%s\n", (const char *)httpres->data);
error:
    return 0;
}

int main()
{
    char escape_hash[128] = {0};
    char url[1024];
    int nsize;

    init();
    
    /*oss_download("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                 "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                 "test/huangkun/188_F775b66f03c37967859B/188_F775b66f03c37967859B.m3u8",
                 "mydownload.m3u8");
     */
    
    /*
    oss_download("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                  "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                  "test/huangkun/188_F775b66f03c37967859B.mkv",
                  "mydownload.m3u8");
     */
    
    oss_upload("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                 "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                 "test/huangkun/test1.m3u8", "download.m3u8");
    
    return 0;
}
