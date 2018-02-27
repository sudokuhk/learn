/*************************************************************************
    > File Name: my_crc.c
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Tue 06 Jan 2015 02:39:57 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

const uint32_t poly = 0xEDB88320;
const uint32_t begin = 0xFFFFFFFF;

uint32_t crc32_1(const uint8_t * data, uint32_t size)
{
    uint32_t reg = 0;
    uint32_t i = 0;
    uint8_t j = 0, k = 0;

    uint8_t * buf = malloc(size + 4);
    memcpy(buf + 0, data, size);
    memcpy(buf + size, (uint8_t *)&reg, sizeof(reg));

    reg = begin;

    if (reg) {
        uint8_t * ptr = (uint8_t *)&reg;
        for (i = 0; i < 4; i++) {
            buf[i] ^= ptr[i] & 0xFF;
        }
    }
    size += 4;
    reg = 0;

    for (i = 0; i < size; i++) {
        for (j = 0; j < 8; j++) {

            uint8_t low_bit = reg & 0x01;
            
            reg = (reg >> 1) | (((buf[i] >> j) & 0x01) << 31);

            if (low_bit) {
                reg ^= poly;
            }
        }
        printf("i = %u, reg = %08x\n", i, reg);
    }
    free(buf);

    return ~reg;
}

uint32_t crc32_2(const uint8_t * data, uint32_t size)
{
    static uint32_t table[255] = {0};
    static uint8_t init = 0;

    uint32_t i = 0;
    uint32_t reg = begin;

    if (!init) {
        init = 1;
        uint8_t j = 0;
        for (i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (j = 0; j < 8; j++) {
                if (crc & 0x01) {
                    crc = (crc >> 1) ^ poly;
                } else {
                    crc >>= 1;
                }
            }
            table[i] = crc;
        }
    }

    for (i = 0; i < size; i++) {
        reg = table[(reg & 0xFF) ^ data[i]] ^ (reg >> 8);
    }

    return ~reg;
}

int main()
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x0a};
    printf("crc32_1 value:0x%08x\n", crc32_1(data, sizeof(data)));
    printf("crc32_2 value:0x%08x\n", crc32_2(data, sizeof(data)));
    return 0;
}
