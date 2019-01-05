/*************************************************************************
    > File Name: btdownload.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed Jan  2 18:56:56 2019
 ************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h> //struct sockaddr_in
#include <netdb.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

typedef unsigned char   uint8_t;
typedef unsigned int    uint32_t;

#define S(n, k)     ((n << k) | (n >> (32 - k)))

#define UINT32(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

const static uint32_t sha1_init[] = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0,
};

void sha1(char * indat, int size, char * digest)
{
    int i, t, n;
    int blocks;
    int len, bits;
    int words;
    int off;
    
    unsigned int msglen = size;
    char * msg_ = (indat);
    unsigned char * msg = (unsigned char *)(msg_);
    
    unsigned int h[5], tp[5];
    unsigned int w[80];
    unsigned int temp;
    
    /* Calculate the number of 512 bit blocks */
    
    len  = msglen + 8/*store bits*/ + 1/*for 0x80*/;
    bits = msglen << 3;
    
    blocks = len >> 6;
    if (len & 0x3F/*2^6-1*/) {
        blocks ++;
    }
    
    words = blocks << 6;
    
    /* clear the padding field */
    memset(msg + msglen + 1, 0x00, words - msglen);
    
    /* insert b1 padding bit */
    msg[msglen] = 0x80;
    
    /* Insert l */
    for (i = 0; i < 4; i++) {
        msg[words - i - 1] = (unsigned char)((bits >> (i * 8)) & 0xFF);
    }
    
    /* Set initial hash state */
    for (i = 0; i < 5; i++) {
        h[i] = sha1_init[i];
    }
    
    for (i = 0; i < blocks; i++) {
        
        /* Prepare the message schedule */
        
        for (t = 0; t < 80; t++) {
            if (t < 16) {
                off = i * 64 + t * 4;
                w[t] = UINT32(msg[off], msg[off + 1],
                              msg[off + 2], msg[off + 3]);
            } else {
                w[t] = S((w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16]), 1);
            }
        }
        
        /* Initialize the five working variables */
        for (t = 0; t < 5; t++) {
            tp[t] = h[t];
        }
        
        /* iterate a-e 80 times */
#define FT1(b, c, d)    ((b & c) | ((~b) & d))
#define FT2(b, c, d)    (b ^ c ^ d)
#define FT3(b, c, d)    ((b & c) | (b & d) | (c & d))
#define FT4(b, c, d)    (b ^ c ^ d)
        
#define K1  0x5a827999
#define K2  0x6ed9eba1
#define K3  0x8f1bbcdc
#define K4  0xca62c1d6
        
#define FUNC(n, i)                                                          \
temp = S(tp[0], 5) + FT##n(tp[1], tp[2], tp[3]) + tp[4] + w[i] + K##n;  \
tp[4] = tp[3]; tp[3] = tp[2]; tp[2] = S(tp[1], 30);     \
tp[1] = tp[0]; tp[0] = temp
        
        for (t = 0; t <= 80; t++) {
            n = t / 20 + 1;
            switch (n) {
                case 1:
                    FUNC(1, t);
                    break;
                case 2:
                    FUNC(2, t);
                    break;
                case 3:
                    FUNC(3, t);
                    break;
                case 4:
                    FUNC(4, t);
                    break;
                    
            }
        }
        
        /* compute the ith intermediate hash value */
        for (t = 0; t < 5; t ++) {
            h[t] = h[t] + tp[t];
        }
    }
    
    for (i = 0; i < 5; i++) {
        for (t = 0; t < 4; t++) {
            digest[i * 4 + t] = (unsigned char)((h[i] >> ((3 - t) * 8)) & 0xFF);
        }
    }
}

int get_info_hash(char * metafile_content, int filesize, char * digest)
{
    int push_pop = 0;
    long i, begin, end;
    
    if(metafile_content == NULL)
        return -1;
    
    const char * sep = strstr(metafile_content, "4:info");
    if (sep == NULL) {
        return -1;
    } 
    begin = (int)(sep - metafile_content) + 6;
    
    i = begin;        // skip "4:info"
    for(; i < filesize; ) {
        if(metafile_content[i] == 'd') { 
            push_pop++;
            i++;
        } else if(metafile_content[i] == 'l') {
            push_pop++;
            i++;
        } else if(metafile_content[i] == 'i') {
            i++;  // skip i
            if(i == filesize)  return -1;
            while(metafile_content[i] != 'e') {
                if((i+1) == filesize)  return -1;
                else i++;
            }
            i++;  // skip e
        } else if((metafile_content[i] >= '0') && (metafile_content[i] <= '9')) {
            int number = 0;
            while((metafile_content[i] >= '0') && (metafile_content[i] <= '9')) {
                number = number * 10 + metafile_content[i] - '0';
                i++;
            }
            i++;  // skip :
            i = i + number;
        } else if(metafile_content[i] == 'e') {
            push_pop--;
            if(push_pop == 0) { end = i; break; }
            else  i++; 
        } else {
            return -1;
        }
    }
    if(i == filesize)  return -1;

    sha1(&metafile_content[begin], end-begin+1, digest);
    
    return 0;
}

std::string format_hash(char * hash, int size)
{
    char buf[1024];
    int off = 0;
    for (int i = 0; i < size; i++) {
        off += sprintf(buf + off, "%%%02x", (unsigned char)hash[i]);
    }
    return std::string(buf, off);
}

int str2ip(const char * szip)
{
    uint32_t ip = 0;
    
    if (szip != NULL) {
        uint32_t a, b, c, d;
        if (sscanf(szip, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            ip = (a << 24) | (b << 16) | (c << 8) | d;
        }
    }
    
    return ip;
}

void ip2str(char * szip, uint32_t iip)
{
    sprintf(szip, "%u.%u.%u.%u",
            (iip >> 24) & 0xFF,
            (iip >> 16) & 0xFF,
            (iip >> 8 ) & 0xFF,
            (iip >> 0 ) & 0xFF);
}

std::string ip2str(int iip)
{
    char szip[64] = {0};
    
    ip2str(szip, iip);
    
    return std::string(szip);
}

std::string nowtime(void)
{
    char date[50];
    time_t t_time = time(NULL);
    struct tm tm_time;
    
    gmtime_r(&t_time, &tm_time);
    strftime(date, 50, "%a, %d %b %Y %H:%M:%S %Z", &tm_time);
    
    return date;
}

std::string get_peer_id(void)
{
    char peer_id[128];
    srand(time(NULL));
    sprintf(peer_id,"-TT1000-%012d", rand());
    
    return peer_id;
}

void * pthread_func(void * arg)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6881);
    addr.sin_addr.s_addr = htonl(str2ip("0.0.0.0"));
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("connect tracker failed\n");
        return NULL;
    }
    if (listen(sock, 128) < 0) {
        printf("listen error<%s>\n", strerror(errno));
        return NULL;
    }
    
    char buf[1024];
    
    while (1) {
        struct sockaddr_in ipv4;
        socklen_t ipv4len = sizeof(ipv4);
        
        int fd = accept(sock, (struct sockaddr *)&ipv4, &ipv4len);
        printf("recv accept:%d\n", fd);
        
        recv(fd, buf, 1024, 0);
        printf("recv buffer:%s\n", buf);
    }
    
    return NULL;
}

static bool isdigit(char c)
{
    if (c >= '0' && c <= '9') {
        return true;
    }
    return false;
}

template <typename T>
static int atoi(const char * dat, int size, T & value)
{
    int n = 0;
    value = 0;
    while (size > 0 && isdigit(*dat)) {
        value = value * 10 + (*dat) - '0';
        dat ++;
        size --;
        n ++;
    }
    return n;
}

void parse_result(const char * buffer, int size)
{
    const char * sep = strstr(buffer, "5:peers");
    if (sep == NULL) {
        printf("no peers valid\n");
        return;
    }
    
    sep += sizeof("5:peers") - 1;
    int npeers = 0;
    int n = atoi(sep, size, npeers);
    printf("npeers:%d:%d\n", npeers, npeers / 6);
    
    sep += n + 1;
    for (int i = 0; i < npeers / 6; i++) {
        int ip = *(int *)sep;
        short port = *(short *)(sep + 4);
        printf("%s:%d\n", (ip2str(ntohl(ip)).c_str()), ntohs(port));
        sep += 6;
    }
}

int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("input bt torrent seed filename\n");
        return -1;
    }
    
    const char * bt_file = argv[1];
    struct stat st;
    if (stat(bt_file, &st) != 0) {
        printf("stat file<%s> error\n", bt_file);
        return -1;
    }
    printf("file length:%d\n", (int)st.st_size);
    
    int fd = open(bt_file, O_RDONLY);
    if (fd < 0) {
        printf("open file<%s> error\n", bt_file);
        return -1;
    }
    
    char * bt_data = new char[st.st_size];
    if (read(fd, bt_data, st.st_size) != st.st_size) {
        printf("read but not all\n");
        delete [] bt_data;
        return -1;
    }
    
    pthread_t pid;
    pthread_create(&pid, NULL, pthread_func, NULL);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = htonl(str2ip("198.177.123.165"));
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("connect tracker failed\n");
        return 0;
    }
    
    char buffer[4096], digest[20];
    int offset = 0;
    //a8bf6c79309f9df6e1111c8eab1870ca259d11f7
    get_info_hash(bt_data, st.st_size, digest);
    std::string sz_hash = format_hash(digest, 20);
    printf("%s\n", sz_hash.c_str());
    
    offset += sprintf(buffer + offset, "GET /announce?peer_id=%s&info_hash=%s&port=6881&left=0&downloaded=0&uploaded=0&compact=1 HTTP/1.0\r\n",
                      get_peer_id().c_str(),
                      sz_hash.c_str());
    offset += sprintf(buffer + offset, "Host: bt.dy20188.com\r\n");
    offset += sprintf(buffer + offset, "Date: %s\r\n", nowtime().c_str());
    offset += sprintf(buffer + offset, "\r\n");
    
    send(sock, buffer, offset, 0);
    int nrecv = recv(sock, buffer, offset, 0);
    printf("%s\n", buffer);
    
    parse_result(buffer, nrecv);
    
    while (1);
    
    return 0;
}
