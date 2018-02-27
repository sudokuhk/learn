/*************************************************************************
 > File Name: crc32.c
 > Author: sudoku.huang
 > Mail: sudoku.huang@gmail.com
 > Created Time: Tue 27 Jan 2018 12:16:40 PM CST
 ************************************************************************/

#include <stdio.h>

typedef unsigned char   uint8_t;
typedef unsigned int    uint32_t;

static uint32_t CRC32[256];
static char init = 0;

static void init_table()
{
    int   i, j;
    uint32_t crc = 0;
    for(i = 0; i < 256; i++) {
        crc = i;
        
        for(j = 0;j < 8;j++) {

            if(crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
        CRC32[i] = crc;
    }
}

int crc32(const char * const indat, int size)
{
    uint32_t ret = 0xFFFFFFFF;
    int i = 0;
    
    if (!init) {
        init_table();
        init = 1;
    }
    
    for(i = 0; i < size;i++) {
        ret = CRC32[((ret & 0xFF) ^ indat[i])] ^ (ret >> 8);
    }
    
    return ~ret;
}

int main(int argc, char * argv[])
{
    //refer: https://1024tools.com/hash
    char input[] = "123456";    // crc32b: 123456 --> 0972d361
    int output = crc32((const char * const)input, 6);
    printf("crc = 0x%08x\n", output);
    
    return 0;
}
