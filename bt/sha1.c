/*************************************************************************
 > File Name: sha1.c
 > Author: sudoku.huang
 > Mail: sudoku.huang@gmail.com
 > Created Time: Fri Feb  9 17:19:36 2018
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

typedef unsigned char   uint8_t;
typedef unsigned int    uint32_t;

#define S(n, k)     ((n << k) | (n >> (32 - k)))

#define UINT32(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

const static uint32_t sha1_init[] = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0,
};

void sha1(char * indat, int size, char * digest)
{
    printf("%c size:%d\n", *indat, size);
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


int find_keyword(char *keyword,long *position, char * metafile_content, int filesize)
{
    long i;
    
    *position = -1;
    if(keyword == NULL)  return 0;
    
    for(i = 0; i < filesize-strlen(keyword); i++) {
        if( memcmp(&metafile_content[i], keyword, strlen(keyword)) == 0 ) {
            *position = i;
            return 1;
        }
    }
    
    return 0;
}

int get_info_hash(char * metafile_content, int filesize)
{
    int   push_pop = 0;
    long  i, begin, end;
    
    if(metafile_content == NULL)
        return -1;
    
    if( find_keyword("4:info",&i, metafile_content, filesize) == 1 ) {
        begin = i+6;  // begin是关键字"4:info"对应值的起始下标
    } else {
        return -1;
    }
    
    i = i + 6;        // skip "4:info"
    for(; i < filesize; )
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
    if(i == filesize)  return -1;
//    
//    SHA1_CTX context;
//    SHA1Init(&context);
//    SHA1Update(&context, &metafile_content[begin], end-begin+1);
//    SHA1Final(info_hash, &context);
    char digest[64] = {0};
    sha1(&metafile_content[begin], end-begin+1, digest);
//#ifdef DEBUG
    printf("info_hash:");
    for(i = 0; i < 20; i++)  
        printf("%02x", (unsigned char)digest[i]);
    printf("\n");
//#endif
    
    return 0;
}

int main(int argc, char * argv[])
{
    /*
     * refer: https://1024tools.com/hash
     * 123456 -> 7c4a8d09ca3762af61e59520943dc26494f8941b
     */
    //char input[1024] = "123456";
//    char input[16] = {0x84, 0xa7, 0xf4, 0x1c, 0x70, 
//        0x18, 0xca, 0x42, 0x2d, 0x5c,
//        0x51, 0xfa, 0x0c, 0xdf, 0xac,
//        0x09,
//    };
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
    
    get_info_hash(bt_data, st.st_size);
    
    char * find = strstr(bt_data, "4:info");
    find += 6;
    int size = st.st_size - (find - bt_data);
    char digest[64] = {0};
    int i, off = 0;
    char hex[64] = {0};
    
    sha1(find, size - 1, digest);
    
    for (i = 0; i < 20; i++) {
        off += sprintf(hex + off, "%02x", (unsigned char)digest[i]);
    }
    printf("%s\n", hex);
    
    delete [] bt_data;
    return 0;
}
