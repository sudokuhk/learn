/*************************************************************************
  > File Name: test.c
  > Author: sudoku.huang
  > Mail: sudoku.huang@gmail.com 
  > Created Time: Tue Mar  6 16:36:15 2018
 ************************************************************************/

#include<stdio.h>

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

static bool is_little_endian(void) {
    static bool check = false;
    static bool little_endian = true;

    if (check) {
        return little_endian;
    }

    union {
        uint32_t    u32;
        struct {
            uint8_t a;
            uint8_t b;
            uint8_t c;
            uint8_t d;
        } s32;
    } flag;

    flag.u32 = 0x00000001;
    printf("u32:%x\n", flag.u32);
    printf("a:%x b:%x c:%x d:%x\n", 
            flag.s32.a,
            flag.s32.b,
            flag.s32.c,
            flag.s32.d);
    check = true;
    little_endian = (flag.s32.a == 0x01);
    return little_endian;
}

const static uint32_t MAGIC = 'hkun';

int main()
{
    printf("little endian:%d\n", is_little_endian());
    printf("%x\n", MAGIC);
    return 0;
}
