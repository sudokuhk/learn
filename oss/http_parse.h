//
//  http_parse.h
//  Learn
//
//  Created by huangkun on 3/18/17.
//
//

#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#define HTTP            "http://"
#define ACCEPT          "Accept: */*"
#define KEEP_ALIVE      "Connection: keep-alive"
//#define SERVER          "Server: AliyunOSS\r\n"
#define AGENT           "User-Agent: Long-Connection/1.0"
//#define AGENT           "User-Agent: aliyun-sdk-c/3.4.0(Compatible Unknown)\r\n"
#define NEWLINE         "\r\n"
#define DEF_TYPE        "application/octet-stream"
#define CONTENT_TYPE    "Content-Type: "
#define CONTENT_LENGTH  "Content-Length: "
#define DATE            "Date: "
#define HTTP_V0         "HTTP/1.0"
#define HTTP_V1         "HTTP/1.1"

void nowtime(char date[50]);

int http_startline(char * buf, int bsize,
                   char ** version, char ** code, char ** reason);

int http_headers(char * buf, int bsize, char ** header);

const char * http_header_content(const char * header);

#endif /* __HTTP_PARSE_H__ */
