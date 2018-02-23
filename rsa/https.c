/*************************************************************************
    > File Name: http.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Fri Feb  9 17:19:36 2018
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define HTTP    "http://"
#define HTTPS   "https://"
#define HTTP_L  (sizeof(HTTP) - 1)
#define HTTPS_L (sizeof(HTTPS) - 1)
#define AGENT           "User-Agent: Long-Connection/1.0"
#define ACCEPT          "Accept: */*"
#define KEEP_ALIVE      "Connection: keep-alive"
#define AGENT           "User-Agent: Long-Connection/1.0"
#define NEWLINE         "\r\n"
#define DEF_TYPE        "application/octet-stream"
#define CONTENT_TYPE    "Content-Type: "
#define CONTENT_LENGTH  "Content-Length: "
#define DATE            "Date: "

#define JUDGE(pch, target)    ((*(pch)) == (target))

typedef unsigned char   uint8;
typedef unsigned int    uint32;
typedef unsigned short  uint16;
typedef uint8           opaque;

static int httpsock = -1;
static int filefd = -1;

//protocol://hostname[:port]/path/[;parameters][?query]#fragment
typedef struct url_t
{
    int     https;
    char    host[1024];
    int     port;
    char    path[1024];
    char    parameters[1024];
    char    query[1024];
    char    fragment[1024];
} url_t;

typedef struct tls_t
{
    opaque  random1[28];
    opaque  random2[28];
    opaque  random3[28];
    opaque  session_id_length;
    opaque  session_id[32];
} tls_t;

url_t urlobj;
tls_t tlsobj;

static char * skip_ws(char *p)
{
    while (*p == ' ') {
        p ++;
    }
    return p;
}

static uint32 get_current_second(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        return tv.tv_sec;
    }
    return -1;
};

static ssize_t atox(const char * str)
{
    ssize_t num = 0;
    int nstr = 0, i, c;
    
    if (str == NULL) {
        return 0;
    }
    
    nstr = strlen(str);
    for (i = 0; i < nstr; i++) {
        c = *(str + i);
        if (isdigit(c)) {
            num = (num << 4) + (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            num = (num << 4) + (c - 'a') + 10;
        } else if (c >= 'A' && c <= 'F') {
            num = (num << 4) + (c - 'A') + 10;
        } else {
            break;
        }
    }
    
    return num;
}

int parsrUrl(const char * szurl)
{
    const char * ptr;
    const char * sep;
    const char * sep1;
    int times = 0;
    
    size_t nurl = strlen(szurl);
    if (nurl <= HTTP_L || (szurl[5] == 's' && nurl <= HTTPS_L)) {
        fprintf(stderr, "invalid url protocol.\n");
        return -1;
    }
    
protocol:
    if (memcmp(HTTP, szurl, HTTP_L) == 0) {
        urlobj.https = 0;
        urlobj.port = 80;
        printf("http access\n");
    } else if (memcmp(HTTPS, szurl, HTTPS_L) == 0) {
        urlobj.https = 1;
        urlobj.port = 443;
        printf("https access\n");
    } else {
        fprintf(stderr, "not http/https.\n");
        goto error;
    }
    
host:
    ptr = szurl + (urlobj.https ? HTTPS_L : HTTP_L);
    sep = strchr(ptr, '/');
    sep1 = strchr(ptr, ':');
    //printf("host:[%s]\n", ptr);

    if (sep != NULL) {
        const char * end = sep;
        if (sep1 != NULL && sep1 < sep) {
            const char * port = sep1;
            urlobj.port = atoi(port + 1);
            end = port;
        } else if (sep1 != NULL) {
            printf("invalid format url.\n");
            goto error;
        }
        memcpy(urlobj.host, ptr, end - ptr);
        ptr = sep + 1;
    } else {
        strcpy(urlobj.host, ptr);
        goto end;
    }
    
    times = 0;
path:
    {
        const char delm[4] = {';', '?', '#', '\0'};
        const char * path = ptr;
        printf("path:[%s]\n", path);
        
        if (times == 4) {
            goto error;
        }
        
        sep = strchr(path, delm[times]);
        if (sep == NULL) {
            times ++;
            goto path;
        }
        memcpy(urlobj.path, path - 1, sep - path + 1);
        ptr = sep + 1;
        
        if (times == 1) {
            goto query;
        } else if (times == 2) {
            goto fragment;
        } else if (times == 3) {
            goto end;
        }
    }
    
    times = 0;
parameters:
    {
        const char delm[3] = {'?', '#', '\0'};
        const char * parameters = ptr;
        
        if (times == 3) {
            goto error;
        }
        
        sep = strchr(parameters, delm[times]);
        if (sep == NULL) {
            times ++;
            goto path;
        }
        memcpy(urlobj.parameters, parameters, sep - parameters);
        ptr = sep + 1;
        
        if (times == 1) {
            goto fragment;
        } else if (times == 2) {
            goto end;
        }
    }
    
query:
    {
        const char * query = ptr;
        sep = strchr(query, '#');
        if (sep == NULL) {
            strcpy(urlobj.query, query);
            goto end;
        } else {
            memcpy(urlobj.query, query, sep - query);
        }
        ptr = sep + 1;
    }
fragment:
    strcpy(urlobj.fragment, ptr);
    
end:
    printf("host:%s\n", urlobj.host);
    printf("port:%d\n", urlobj.port);
    printf("path:%s\n", urlobj.path);
    printf("parameters:%s\n", urlobj.parameters);
    printf("query:%s\n", urlobj.query);
    printf("fragment:%s\n", urlobj.fragment);

    return 0;
error:
    return -1;
}

void ip2str(char * szip, uint32_t iip)
{
    sprintf(szip, "%u.%u.%u.%u",
            (iip >> 24) & 0xFF,
            (iip >> 16) & 0xFF,
            (iip >> 8 ) & 0xFF,
            (iip >> 0 ) & 0xFF);
}

int dns_resolver(const char * const host, int port, struct sockaddr * addr)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char szport[10];
    time_t t;
    int res = -1;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family     = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype   = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags      = 0;
    hints.ai_protocol   = 0;          /* Any protocol */
    
    t = time(NULL);
    res = getaddrinfo(host, "80", &hints, &result);
    if (res != 0) {
        printf("getaddrinfo failel! %s->%s\n", host, gai_strerror(res));
        return -1;
    }
    printf("dns resolver:%d\n", (int)(time(NULL) - t));
    
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) { //always true.
            *addr = *rp->ai_addr;
            break;
        }
    }
    freeaddrinfo(result);
    
    struct sockaddr_in * inaddr = (struct sockaddr_in *)addr;
    char ip[64] = {0};
    
    inaddr->sin_port = htons(port);
    ip2str(ip, ntohl(inaddr->sin_addr.s_addr));
    printf("dns resolver:<%s> -> <%s>\n", host, ip);
    
    return 0;
}

void setnoblock(int fd, int block)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (block) {
        flag |= O_NONBLOCK;
    } else {
        flag &= ~O_NONBLOCK;
    }
    fcntl(fd, F_SETFL, flag);
}

int closefd(int * pfd)
{
    printf("closefd\n");
    if (pfd != NULL && *pfd > 0) {
        close(*pfd);
    }
    *pfd = -1;
    
    return 0;
}

int open_connect(int * _sock, struct sockaddr * addr, int timeo/*s*/)
{
    fd_set rdset, wset;
    struct timeval to;
    int active = 0, error;
    socklen_t len = sizeof(socklen_t);
    int sock = -1;
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        goto error;
    }
    
    setnoblock(sock, 1);
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
        error = 0;
        if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0
           || error != 0) {
            printf("connect error select\n");
            goto error;
        }
    }
    printf("connect succeed!\n");
    *_sock = sock;
    return 0;
    
error:
    if (sock > 0) {
        closefd(&sock);
    }
    *_sock = -1;
    return -1;
}

void nowtime(char date[50])
{
    time_t t_time = time(NULL);
    struct tm tm_time;
    
    gmtime_r(&t_time, &tm_time);
    strftime(date, 50, "%a, %d %b %Y %H:%M:%S %Z", &tm_time);
}

int readn(int *pfd, char * buf, int bsize, int timeo)
{
    fd_set rdset;
    struct timeval to;
    int nread = 0;
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
                return -1;
            }
        } else if (nread == 0) {
            printf("read error:%d, %s\n", errno, strerror(errno));
            return -1;
        }
        return nread;
    }
    
    return 0;
}

int readnb(int *pfd, char * buf, int bsize)
{
    char * ptr = buf;
    char * pend = buf + bsize;
    int nread = 0;
    
    while (ptr < pend) {
        nread = readn(pfd, ptr, pend - ptr, 10);
        if (nread < 0) {
            return -1;
        }
        ptr += nread;
    }
    return bsize;
}

int writen(int *sock, const char * data, int dsize)
{
    int nwrite = 0;
    int total  = 0;
    
    while (total < dsize) {
        nwrite = write(*sock, data + total, dsize - total);
        if (nwrite < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                usleep(10/*ms*/);
                continue;
            }
            printf("write error:%d, %s\n", errno, strerror(errno));
            goto failed;
        }
        total += nwrite;
    }
    
    return dsize;
    
failed:
    return -1;
}

int send_httprequest_by_length(int * sock, struct url_t * url)
{
    char buf[1024] = {0};
    char path[1024] = {0};
    char date[50];
    int off = 0;
    
    if (url->path[0] == '\0') {
        off += sprintf(path + off, "/index");
    } else {
        off += sprintf(path + off, "/%s", url->path);
    }
    
    if (url->parameters[0] != '\0') {
        off += sprintf(path + off, ";%s", url->parameters);
    }
    
    if (url->query[0] != '\0') {
        off += sprintf(path + off, "?%s", url->query);
    }
    
    if (url->fragment[0] != '\0') {
        off += sprintf(path + off, "#%s", url->fragment);
    }
    
    nowtime(date);
    off = 0;
    off += sprintf(buf + off, "GET /%s HTTP/1.1" NEWLINE, path);
    off += sprintf(buf + off, "Host: %s" NEWLINE, urlobj.host);
    off += sprintf(buf + off, "Date: %s" NEWLINE, date);
    off += sprintf(buf + off, "%s" NEWLINE, AGENT);
    off += sprintf(buf + off, "%s" NEWLINE, ACCEPT);
    off += sprintf(buf + off, "%s" NEWLINE, KEEP_ALIVE);
    off += sprintf(buf + off, NEWLINE);
    
    return writen(sock, buf, off);
}

int recv_httpresponse(int * sock)
{
    const ssize_t buffer_size = 4096;
    char buffer[buffer_size + 1];
    ssize_t nread = 0, ntotal = 0;
    char * pbuffer = buffer, * pbufferend = pbuffer + buffer_size;
    int bstartline = 0, bhead = 0, bchunk = 0, berror = 0;
    size_t bodylength = 0;
    char * sepnewline = NULL, * pconsume = buffer;
    ssize_t code = 200;
    
    goto read;
parse:
    if (!bstartline) {
        sepnewline = strchr(buffer, '\n');
        if (sepnewline == NULL || !JUDGE(sepnewline - 1, '\r')) {
            goto read;
        }
        
        char * sep0 = strchr(buffer, ' ');
        if (sep0 == NULL || sep0 > sepnewline) {
            goto error;
        }
        char * sep1 = strchr(sep0 + 1, ' ');
        if (sep1 == NULL || sep1 > sepnewline) {
            goto error;
        }
        
        *sep0 = *sep1 = *(sepnewline - 1)= '\0';
        code = atoll(sep0 + 1);
        const char * version = buffer;
        const char * result = sep1 + 1;

        printf("start line:[%s] [%zd] [%s]\n", version, code, result);
        printf("------ parse startline end ------\n\n");
        bstartline = 1;
        pconsume = sepnewline + 1;  //skip \r\n
    }
   
head:
    if (!bhead) {
        sepnewline = strchr(pconsume, '\n');
        if (sepnewline == NULL) {
            goto read;
        }
        
        if (!JUDGE(sepnewline - 1, '\r')) {
            goto error;
        }
        
        if (JUDGE(sepnewline + 1, '\r') && JUDGE(sepnewline + 2, '\n')) {
            bhead = 1;
            sepnewline += 2;
            printf("------- http head parse end -----------\n\n");
        } else {
            *(sepnewline - 1) = '\0';
            printf("[%s]\n", pconsume);
            
            char * sep = strchr(pconsume, ':');
            if (sep == NULL) {
                goto error;
            }
            *sep = '\0';
            if (strcasecmp(pconsume, "Content-Length") == 0) {
                bodylength = atoll(sep + 1);
                printf("--->CONTENT_LENGTH:%zd\n", bodylength);
            } else if (strcasecmp(pconsume, "Transfer-Encoding") == 0) {
                const char * value = skip_ws(sep + 1);
                if (strcasecmp(value, "chunked") == 0) {
                    bchunk = 1;
                    printf("chunk mode\n");
                }
            }
        }
        pconsume = sepnewline + 1;
        
        if (bhead) {
            if (bchunk) {
                goto chunk;
            } else {
                goto data;
            }
        }
        goto head;
    }
    
    ntotal = 0;
data:
    {
        ssize_t datasize = pbuffer - pconsume;
        if (datasize > 0) {
            ssize_t need = bodylength - ntotal;
            ntotal += need;
            printf("this time:%zd, total:%zd\n", datasize, ntotal);
            if (code == 200) {
                write(filefd, pconsume, need);
            } else {
                char error[4096];
                memcpy(error, pconsume, need);
                error[need] = '\0';
                fprintf(stderr, "%s", error);
            }
            pconsume += need;
        
            if (ntotal == bodylength) {
                if (!bchunk) {
                    goto done;
                }
                if (code != 200) {
                    printf("\n");
                }
            } else if (ntotal < bodylength) {
                goto read;
            }
        } else {
            goto read;
        }
    }
    
chunk:
    {
        printf("read chunk\n");
        char * sep = strchr(pconsume, '\n');
        if (sep == NULL) {
            goto read;
        }
        if (!JUDGE(sep - 1, '\r')) {
            goto error;
        }
        
        *(sep - 1) = '\0';
        bodylength = atox(pconsume);
        printf("read chunk size:%zd\n", bodylength);
        ntotal = 0;
        pconsume = sep + 1;
        
        if (bodylength > 0) {
            goto data;
        } else {
            goto done;
        }
    }
read:
    //reset buffer.
    printf("read, reset buffer, consume:%zd, remain:%zd\n",
           pconsume - buffer, pbuffer - pconsume);
    if (pconsume != pbuffer || pconsume != buffer) {
        printf("read, memmove\n");
        memmove(buffer, pconsume, pbuffer - pconsume);
        pbuffer = buffer + (pbuffer - pconsume);
    } else {
        pbuffer = buffer;
    }
    printf("read, reset buffer done, remain:%zd\n", pbufferend - pbuffer);
    if ((nread = readn(sock, pbuffer, pbufferend - pbuffer, 10)) < 0) {
        goto error;
    }
    printf("nread:%zd\n", nread);
    
    pbuffer[nread] = '\0';
    pbuffer += nread;
    pconsume = buffer;
    
    if (nread == 0) {
        goto read;
    } else if (!bstartline && !bhead) {
        goto parse;
    } else if (bchunk) {
        goto chunk;
    } else {
        goto data;
    }
done:
    printf("read response done\n");
    return 0;
error:
    return -1;
}

void do_http(int * sock, struct url_t * url)
{
    // 1. send http request
    send_httprequest_by_length(sock, url);
    
    // 2. recv http response
    recv_httpresponse(sock);
}

#pragma mark -- tls begin
/***************tls**********/
enum en_tls_handshake_type {
    en_tls_hello_request        = 0,
    en_tls_client_hello         = 1,
    en_tls_server_hello         = 2,
    en_tls_certificate          = 11,
    en_tls_server_key_exchange  = 12,
    en_tls_certificate_request  = 13,
    en_tls_server_hello_done    = 14,
    en_tls_certificate_verify   = 15,
    en_tls_client_key_exchange  = 16,
    en_tls_finished             = 20,
    en_tls_unknown              = 255,
} ;

enum en_tls_hash_algorithm
{
    en_tls_hash_none = 0,
    en_tls_hash_md5 = 1,
    en_tls_hash_sha1 = 2,
    en_tls_hash_sha224 = 3,
    en_tls_hash_sha256 = 4,
    en_tls_hash_sha384 = 5,
    en_tls_hash_sha512 = 6,
};

enum en_tls_signature_algorithm
{
    en_tls_signature_anonymous = 0,
    en_tls_signature_rsa = 1,
    en_tls_signature_dsa = 2,
    en_tls_signature_ecdsa = 3,
};

enum en_tls_content_type
{
    en_tls_change_cipher_spec = 20,
    en_tls_alert = 21,
    en_tls_handshake = 22,
    en_tls_application_data = 23,
};

/*
enum {
    change_cipher_spec(20), alert(21), handshake(22),
    application_data(23), (255)
} ContentType;
struct {
    ContentType type;
    ProtocolVersion version;
    uint16 length;
    opaque fragment[TLSPlaintext.length];
} TLSPlaintext;
struct {
    ContentType type;
    ProtocolVersion version;
    uint16 length;
    opaque fragment[TLSCompressed.length];
} TLSCompressed;
struct {
    ContentType type;
    ProtocolVersion version;
    uint16 length;
    select (SecurityParameters.cipher_type) {
        case stream: GenericStreamCipher;
        case block:  GenericBlockCipher;
        case aead:   GenericAEADCipher;
    } fragment;
} TLSCiphertext;
stream-ciphered struct {
    opaque content[TLSCompressed.length];
    opaque MAC[SecurityParameters.mac_length];
} GenericStreamCipher;
 struct {
     opaque IV[SecurityParameters.record_iv_length];
     block-ciphered struct {
         opaque content[TLSCompressed.length];
         opaque MAC[SecurityParameters.mac_length];
         uint8 padding[GenericBlockCipher.padding_length];
         uint8 padding_length;
     };
 } GenericBlockCipher;
 struct {
     opaque nonce_explicit[SecurityParameters.record_iv_length];
     aead-ciphered struct {
        opaque content[TLSCompressed.length];
     };
 } GenericAEADCipher;
 */
typedef struct tls_protocol_version_t {
    uint8 major;
    uint8 minor;
} tls_protocol_version_t;

typedef struct tls_plain_text_t
{
    enum en_tls_content_type    type;
    tls_protocol_version_t      version;
    uint16                      length;
    opaque                      fragment[0];
} tls_plain_text_t;

typedef struct tls_compressed_text_t
{
    enum en_tls_content_type     type;
    tls_protocol_version_t  version;
    uint16                  length;
    opaque                  fragment[0];
} tls_compressed_text_t;

typedef struct tls_cipher_text_t
{
    enum en_tls_content_type     type;
    tls_protocol_version_t  version;
    uint16                  length;
    //
    opaque                  fragment[0];
} tls_cipher_text_t;

typedef struct tls_signature_hash_t
{
    opaque  hash;
    opaque  signature;
}tls_signature_hash_t;

typedef struct tls_hello_request_t
{
} tls_hello_request_t;

typedef struct var_array_t
{
    uint32      array_size;
    uint32      element_size;
    uint8 *     array;
} var_array_t;

typedef struct tls_random_t {
    uint32_t        gmt_unix_time;      //seconds since the midnight starting Jan 1, 1970, UTC, ignoring leap seconds
    opaque          random_bytes[28];   //28 bytes generated by a secure random number generator
} tls_random_t;

//typedef uint8[2]    cipher_suite_t;

typedef struct tls_extension_t {
    uint16      type;
    uint16      length;
    opaque      data;
} extension_t;

typedef struct tls_client_hello_t
{
    tls_protocol_version_t  client_version;
    tls_random_t            random;
    var_array_t             session_id;             //<0..32>
    var_array_t             cipher_suites;          //cipher_suites<2..2^16-2>
    var_array_t             compression_methods;    //<1..2^8-1>
    //extensions_present
    //Extension extensions<0..2^16-1>;
} tls_client_hello_t;

typedef struct tls_server_hello_t {
    tls_protocol_version_t  server_version;
    tls_random_t            random;
    var_array_t             session_id;             //<0..32>
    opaque                  cipher_suite[2];
    opaque                  compression_method;    //<1..2^8-1>
    //extensions_present
    //Extension extensions<0..2^16-1>;
} tls_server_hello_t;

typedef struct tls_certificate_t {
} tls_certificate_t;

typedef struct  {
} tls_server_key_exchange_t;

typedef struct  {
} tls_certificate_request_t;

typedef struct  {
} tls_server_hello_done_t;

typedef struct  {
} tls_certificate_verfiy_t;

typedef struct  {
} tls_client_key_exchange_t;

typedef struct  {
} tls_finished_t;

typedef struct tls_handshake_t {
    uint8_t     msg_type;
    uint32_t    length;             /* bytes in message, need uint24 */
    union {
        tls_hello_request_t          hello_request;
        tls_client_hello_t           client_hello;
        tls_server_hello_t           server_hello;
        tls_certificate_t            certificate;
        tls_server_key_exchange_t    server_key_exchange;
        tls_certificate_request_t    certificate_request;
        tls_server_hello_done_t      server_hello_done;
        tls_certificate_verfiy_t     certificate_verfiy;
        tls_client_key_exchange_t    client_key_exchange;
        tls_finished_t               finished;
    } body;
} tls_handshake_t;

typedef struct rdnSequence_t
{
    
} rdnSequence_t;

typedef struct validity_t
{
    uint32  notBefore;
    uint32  notAfter;
} validity_t;

typedef struct subjectPublicKeyInfo_t
{
    uint8   alg;
} subjectPublicKeyInfo_t;

typedef struct x509v3_t
{
    uint8           version;
    uint8           serialNumber[128];
    uint8           signature[128];
    
    uint8           issuer_n;
    rdnSequence_t   issuer[128];
    
    validity_t      validity;
    
    uint8           subject_n;
    rdnSequence_t   subject[128];
    
    
} x509v3_t;

static int send_tls_client_hello(int * sock)
{
    uint8 buf[4096] = {0};
    uint32 i = 0, j;
    uint32 seconds = get_current_second();
    
    printf("\n\n************************\n");
    printf("send tls client hello now seconds:%u\n", seconds);
    
    buf[i ++] = en_tls_handshake;           //content type, handshake
    buf[i ++] = 0x03;   buf[i ++] = 0x01;   //version TLS 1.0
    buf[i ++] = 0x00;   buf[i ++] = 0x00;   //Length
    
    //fragment
    //Handshake type, Client Hello
    buf[i ++] = 0x01;
    
    //Length
    buf[i ++] = 0;
    buf[i ++] = 0;
    buf[i ++] = 0;
    
    //Version, TLS 1.2
    buf[i ++] = 0x03;
    buf[i ++] = 0x03;
    
    //random
    buf[i ++] = (seconds >> 24) & 0xFF;
    buf[i ++] = (seconds >> 16) & 0xFF;
    buf[i ++] = (seconds >> 8) & 0xFF;
    buf[i ++] = (seconds >> 0) & 0xFF;
 
    printf("generate:random:\n");
    for (j = 0; j < 28; j++) {
        tlsobj.random1[j] = random() & 0xFF;
        printf("0x%02x ", tlsobj.random1[j]);
        buf[i ++] = tlsobj.random1[j];
    }
    printf("\n");
    
    //session id
    buf[i ++] = 0;
    
    //cipher suites length.  1->TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
    buf[i ++] = 0x00;
    buf[i ++] = 0x02;
    
    buf[i ++] = 0xc0;   //TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
    buf[i ++] = 0x2f;
    
    //compression methods.
    buf[i ++] = 1;
    buf[i ++] = 0;
    
    buf[3] = ((i - 5) >> 8) & 0xFF;
    buf[4] = ((i - 5) >> 0) & 0xFF;
    
    buf[6] = ((i - 9) >> 16) & 0xFF;
    buf[7] = ((i - 9) >> 8) & 0xFF;
    buf[8] = ((i - 9) >> 0) & 0xFF;
    printf("************************\n");
    
    return writen(sock, (const char *)buf, i);
}

static int recv_tls_header(int * sock, enum en_tls_content_type type)
{
    opaque buf[10] = {0};
    int nbody = -1;
    
    printf("recv tls header, want type:%d\n", type);
    if (readnb(sock, (char *)buf, 5) < 0) {
        fprintf(stderr, "recv tls header error\n");
        return -1;
    }
    
    opaque content_type = buf[0];
    if (content_type != type) {
        fprintf(stderr, "content type not match, want:%u, act:%u\n",
                type, content_type);
        return -1;
    }
    
    if (buf[1] == 0x03) {
        printf("TLS Version:1.%u\n", buf[2] - 1);
    }
    
    nbody = (buf[3] << 8) | buf[4];
    printf("body length:%d\n", nbody);
    
    return nbody;
}

static int recv_tls_server_hello(int * sock)
{
    opaque buf[4096] = {0};
    uint32 i, j;
    int nbody = 0;
    uint32 ntext = 0;
    ssize_t nread = 0;
    
    printf("\n\n************************\n");
    printf("receive tls server hello\n");
    nbody = recv_tls_header(sock, en_tls_handshake);
    if (nbody < 0) {
        fprintf(stderr, "serverhello, recv header error\n");
        return -1;
    }
    
    //read body
    if ((nread = readnb(sock, (char *)buf, nbody)) < 0) {
        fprintf(stderr, "recv server hello, body error\n");
        return -1;
    }
    
    i = 0;
    opaque handshake_type = buf[i ++];
    ntext = (buf[i ++] << 16);
    ntext |= (buf[i ++] << 8);
    ntext |= buf[i ++];
    
    i += 2; //skip version
    i += 4; //skip GMT Unix Time
    for (j = 0; j < 28; j++) {
        tlsobj.random2[j] = buf[i ++];
    }
    
    tlsobj.session_id_length = buf[i ++];
    for (j = 0; j < tlsobj.session_id_length; j++) {
        tlsobj.session_id[j] = buf[i ++];
    }
    
    opaque cipher_h = buf[i ++];
    opaque cipher_l = buf[i ++];
    if (cipher_h == 0xc0 && cipher_l == 0x2f) {
        printf("cipher suite match\n");
    } else {
        printf("cipher suite not match\n");
        return -1;
    }
    printf("************************\n");
    
    return 0;
}

static int tls_x509v3_parse(const opaque * buf, int size, x509v3_t * x509)
{
    uint8 tag;      //universal(00), application(01), context-specific(10), private(11)
    uint8 objtype;  //struct(1)  simple data type(0)
    uint8 datatype;
    uint32 datalen, bytecnt, i, j;
    printf("parse x509\n");
    
data:
    tag         = (buf[0] >> 6) & 0x03;
    objtype     = (buf[0] >> 5) & 0x01;
    datatype    = (buf[0]) & 0x1f;
    datalen     = buf[1] & 0x7f;
    bytecnt     = 0;
    
    printf("tag:%u, objtype:%u, datatype:%u, datalen:%u\n", tag, objtype, datatype, datalen);
    
    if (buf[1] & 0x80) {
        buf += 2;
        for (i = 0; i < datalen; i++) {
            bytecnt = (bytecnt << 8) | buf[i];
        }
        buf += datalen;
    } else {
        buf += 2;
        bytecnt = datalen;
    }
    printf("bytecnt:%u\n", bytecnt);
    
    return 0;
}

static int recv_tls_certificate(int * sock)
{
    opaque buf[4096] = {0};
    uint32 i, j, certificate_count = 0;
    int nbody = 0;
    uint32 ntext = 0, ncertificate = 0;
    ssize_t nread = 0;
    
    printf("\n\n************************\n");
    printf("reveive tls certificate\n");
    nbody = recv_tls_header(sock, en_tls_handshake);
    if (nbody < 0) {
        fprintf(stderr, "certificate, recv header error\n");
        return -1;
    }
    
    //read body
    if ((nread = readnb(sock, (char *)buf, nbody)) < 0) {
        fprintf(stderr, "certificate, recv body error\n");
        return -1;
    }
    
    i = 0;
    //Handshake type, Certificate
    opaque type = buf[0];
    if (type != en_tls_certificate) {
        fprintf(stderr, "certificate, type not match, want:%u, recv:%u\n",
                en_tls_certificate, type);
        return -1;
    }
    i ++;
    
    //Length, 3 Bytes.
    ntext = ((buf[i]) << 16) | ((buf[i + 1]) << 8) | (buf[i + 2]);
    i += 3;
    printf("certificate, tls plain text Lenght:%u\n", ntext);
    
    //Certificate Length, 3 Bytes
    ncertificate = ((buf[i]) << 16) | ((buf[i + 1]) << 8) | (buf[i + 2]);
    i += 3;
    printf("certificate length:%u\n", ncertificate);
    
    while (ncertificate > 0) {
        uint32 one_ceritificate = (buf[i] << 16) | (buf[i + 1] << 8) | buf[i + 2];
        i += 3;
        
        printf("certificate:%d, size:%u\n", ++ certificate_count, one_ceritificate);
        x509v3_t x509v3;
        if (tls_x509v3_parse(&buf[i], one_ceritificate, &x509v3) < 0) {
            printf("certificate, parse x509v3 error\n");
            return -1;
        }
        i += one_ceritificate;
        
        ncertificate -= (one_ceritificate + 3);
        printf("remaining:%u\n", ncertificate);
    }
    printf("************************\n");
    
    return 0;
}

static int recv_tls_server_key_exchange(int * sock)
{
    opaque buf[4096] = {0};
    uint32 i, j;
    int nbody = 0;
    uint32 ntext = 0;
    ssize_t nread = 0;
    
    printf("\n\n************************\n");
    printf("reveive tls server key exchange\n");
    nbody = recv_tls_header(sock, en_tls_handshake);
    if (nbody < 0) {
        fprintf(stderr, "server key exchange, recv header error\n");
        return -1;
    }
    
    //read body
    if ((nread = readnb(sock, (char *)buf, nbody)) < 0) {
        fprintf(stderr, "server key exchange, recv body error\n");
        return -1;
    }
    
    i = 0;
    //Handshake type, Server Key Exchange
    opaque type = buf[0];
    if (type != en_tls_server_key_exchange) {
        fprintf(stderr, "server key exchange, type not match, want:%u, recv:%u\n",
                en_tls_server_key_exchange, type);
        return -1;
    }
    i ++;
    
    ntext = ((buf[i]) << 16) | ((buf[i + 1]) << 8) | (buf[i + 2]);
    i += 3;
    printf("server key exchange, tls plain text Lenght:%u\n", ntext);
    printf("************************\n");
    
    return 0;
}

static int recv_tls_server_hello_done(int * sock)
{
    opaque buf[4096] = {0};
    uint32 i, j;
    int nbody = 0;
    uint32 ntext = 0;
    ssize_t nread = 0;
    
    printf("\n\n************************\n");
    printf("reveive tls server hello done\n");
    nbody = recv_tls_header(sock, en_tls_handshake);
    if (nbody < 0) {
        fprintf(stderr, "server hello done, recv header error\n");
        return -1;
    }
    
    //read body
    if ((nread = readnb(sock, (char *)buf, nbody)) < 0) {
        fprintf(stderr, "server hello done, recv body error\n");
        return -1;
    }
    
    i = 0;
    //Handshake type, Server Hello Done
    opaque type = buf[0];
    if (type != en_tls_server_hello_done) {
        fprintf(stderr, "server hello done, type not match, want:%u, recv:%u\n",
                en_tls_server_key_exchange, type);
        return -1;
    }
    i ++;
    
    ntext = ((buf[i]) << 16) | ((buf[i + 1]) << 8) | (buf[i + 2]);
    i += 3;
    printf("server hello done, tls plain text Lenght:%u\n", ntext);
    printf("************************\n");
    
    return 0;
}

static int do_tls_shake(int * sock, struct url_t * url)
{
    if (send_tls_client_hello(sock) < 0) {
        fprintf(stderr, "send client hello failed\n");
        return -1;
    }
    
    if (recv_tls_server_hello(sock) < 0) {
        fprintf(stderr, "recv server hello failed\n");
        return -1;
    }
    
    if (recv_tls_certificate(sock) < 0) {
        fprintf(stderr, "recv server certificate failed\n");
        return -1;
    }
    
    if (recv_tls_server_key_exchange(sock) < 0) {
        fprintf(stderr, "recv server key exchange failed\n");
        return -1;
    }
    
    if (recv_tls_server_hello_done(sock) < 0) {
        fprintf(stderr, "recv server hello done failed\n");
        return -1;
    }
    
    return -1;
}
#pragma mark -- tls end
/***************tls**********/

int main(int argc, char * argv[])
{
    const char * filename = NULL;
    struct sockaddr addr;
#if 1
    const char * szurl = NULL;
    szurl = "http://www.imailtone.com:80/WebApplication1/WebForm1.aspx;aabbcc?name=tom&age=20#resume";
    szurl = "http://ruigongkao.oss.ruijy.cn/image/source/image/68047fa494a277d3dcfdd153b4c395c0.jpg";
    szurl = "http://service.cloud.ping-qu.com/api/tc/job/feedback";
    szurl = "https://ruigongkao.oss.ruijy.cn/image/source/image/68047fa494a277d3dcfdd153b4c395c0.jpg";
#else
    if (argc < 2) {
        fprintf(stderr, "invalid input!\n");
        fprintf(stderr, "Usage: %s url\n", argv[1]);
        exit(0);
    }
    const char * szurl = argv[1];
#endif
    memset(&urlobj, 0x00, sizeof(urlobj));
    memset(&addr, 0x00, sizeof(addr));
    memset(&tlsobj, 0x00, sizeof(tlsobj));
    
    if (parsrUrl(szurl) != 0) {
        printf("invalid url:%s\n", szurl);
        exit(0);
    }
    
    filename = "index";
    if (urlobj.path[0] != '\0') {
        const char * sep = strrchr(urlobj.path, '/');
        if (sep == NULL) {
            filename = urlobj.path;
        } else {
            filename = sep + 1;
        }
    }
    printf("download filename:<%s>\n", filename);
    filefd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (filefd < 0) {
        fprintf(stderr, "open file<%s> error:<%s>\n", filename, strerror(errno));
        exit(0);
    }
    
    if (dns_resolver(urlobj.host, urlobj.port, &addr) != 0) {
        fprintf(stderr, "dns resolver error.<%s>\n", urlobj.host);
        exit(0);
    }
    
    if (open_connect(&httpsock, &addr, 10) < 0) {
        fprintf(stderr, "connect to host<%s> error\n", urlobj.host);
        exit(0);
    }
    
    if (urlobj.https) {
        do_tls_shake(&httpsock, &urlobj);
    }
    //do_http(&httpsock, &urlobj);
    
    usleep(10000000);
    closefd(&httpsock);
    closefd(&filefd);
    
    return 0;
}
