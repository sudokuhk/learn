/*************************************************************************
  > File Name: crc.c
  > Author: sudoku.huang
  > Mail: sudoku.huang@gmail.com 
  > Created Time: Tue 06 Jan 2015 12:16:40 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned char uchar;

static uint   CRC32[256];
static char   init = 0;

//初始化表
static void init_table()
{
    int   i,j;
    uint   crc;
    for(i = 0;i < 256;i++)
    {
        crc = i;
        for(j = 0;j < 8;j++)
        {
            if(crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc = crc >> 1;
            }
        }
        CRC32[i] = crc;
    }
}

//crc32实现函数
uint crc32( uchar *buf, int len)
{
    uint ret = 0xFFFFFFFF;
    int   i;
    if( !init )
    {
        init_table();
        init = 1;
    }
    for(i = 0; i < len;i++)
    {
        ret = CRC32[((ret & 0xFF) ^ buf[i])] ^ (ret >> 8);
        printf("ret = %08x\n", ret);
    }
    ret = ~ret;
    return ret;
}

int main()
{
    char data[] = {0x01, 0x02, 0x03, 0x04, 0x0a};
    printf("crc32 value:0x%08x\n", crc32(data, sizeof(data)));

    return 0;
}
