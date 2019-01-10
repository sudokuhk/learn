#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

unsigned char ARRAY[0x100]; //256
unsigned int KEY[0x20]; //32
unsigned int init = 0;

void dump_mem(const char * mem, int len)
{
    const unsigned char * p = (const unsigned char *)mem;
    for (int i = 0, j = 1; i < len; i ++, j++) {
        printf("0x%02x ", *(p + i));
        if (j % 8 == 0) {
            printf("\n");
        }        
    }
    printf("\n");
}

void dump_mem2str(char * str, const unsigned char * mem, int len)
{
    int off = 0;
    for (int i = 0; i < len; i++) {
        off += sprintf(str + off, "%02x", *(unsigned char *)(mem + i));
    }
}

int load(const char * fn, void * mem)
{
    struct stat st;
    if (stat(fn, &st) < 0) {
        printf("stat file:%s error\n", fn);
        return -1;
    }
    
    int fd = open(fn, O_RDONLY);
    if (fd < 0) {
        printf("open error:%s\n", fn);
        return -1;
    }
    
    read(fd, mem, st.st_size);
    close(fd);
    
    return st.st_size;
}

unsigned int encrypt_dword_0(unsigned int d) 
{
    unsigned char b[4] = {0};
    const unsigned char * pd = (const unsigned char *)&d;
    for (int i = 0; i < 4; i++) {
        b[i] = *(pd + i);
    }
    
    unsigned int ret = 0;
    for (int i = 0; i < 4; i++) {
        ret = (ret << 4) + b[i];    //right shift 4 ? why not 8???
    }
    
    return ret;
}

unsigned int encrypt_dword_1(unsigned int d) 
{
    unsigned int pre = 0;
    unsigned char b[4] = {0};
    const unsigned char * pd = (const unsigned char *)&d;
    for (int i = 0; i < 4; i++) {
        b[i] = *(pd + i);
    }
    
    unsigned int v0, v1, v2, v3, v4;
    for (int i = 0; i < 4; i++) {
        switch (i & 0x01) {
            case 0: 
                v0 = pre << 7;
                v1 = b[i] ^ v0;
                v2 = pre >> 3;
                v3 = v1 ^ v2;
                pre = pre ^ v3;
                break;
            case 1:
                v0 = pre << 11;
                v1 = b[i] ^ v0;
                v2 = pre >> 5;
                v3 = v1 ^ v2;
                v4 = ~v3;   //same as v4 = v3 ^ 0xFFFFFFFF;
                pre = pre ^ v4;
                break;
            default:    //no default.
                break;
        }
    }
    
    return pre & 0x7FFFFFFF;
}

unsigned int encrypt_dword_2(unsigned int d)
{
    unsigned int pre = 0x4E67C6A7;
    unsigned char b[4] = {0};
    const unsigned char * pd = (const unsigned char *)&d;
    for (int i = 0; i < 4; i++) {
        b[i] = *(pd + i);
    }
    
    unsigned int v0, v1, v2, v3, v4;
    for (int i = 0; i < 4; i++) {
        v0 = pre << 5;
        v1 = v0 + b[i];
        v2 = pre >> 2;
        v3 = v1 + v2;
        
        pre = pre ^ v3;
    }
    
    return pre;
}

unsigned int encrypt_dword_3(unsigned int d)
{
    unsigned int v1, v2, v3, v4;
    
    v1 = ARRAY[(d >> 24) & 0xFF];
    v2 = ARRAY[d & 0xFF];
    v3 = (ARRAY[(d >> 16) & 0xFF] << 16) | (v1 << 24);
    v4 = v3 | (ARRAY[(d >> 8) & 0xFF] << 8);
    
    return (((v4 | v2) << 10) | (v3 >> 22)) ^ ((v4 >> 8) | (v2 << 24)) ^ (v4 | v2)
    ^ (4 * (v4 | v2) | (v1 >> 6)) ^ (((v4 | v2) << 18) | (v4 >> 14));
}

unsigned int rendian(unsigned int d)
{
    unsigned char b[4] = {0};
    const unsigned char * pd = (const unsigned char *)&d;
    for (int i = 0; i < 4; i++) {
        b[i] = *(pd + i);
    }
    
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

unsigned char rbit(unsigned char c) 
{
    unsigned char ret = 0, bit = 7;
    while (c > 0) {
        if (c & 0x01) {
            ret |= (1 << bit);
        }
        c >>= 1;
        bit --;
    }
    
    return ret;
}

//unsigned int rbit_eachbyte(unsigned int d)
unsigned int rint(unsigned int d)
{
    const unsigned char * c = (const unsigned char *)&d;
    unsigned int ret = 0;
    
    for (int i = 0; i < 4; i++) {
        ret |= rbit(c[i]) << (8 * i);
    }
    
    return ret;
}

void encrypt_dword_array_0(unsigned int * array)
{
    unsigned int _arr[4] = {0};
    for (int i = 0; i < 4; i++) {
        _arr[i] = rendian(array[i]);
    }
    
    for (int i = 0; i < 32; i++) {
        unsigned int value = _arr[(i + 1) % 4];
        value ^= _arr[(i + 2) % 4];
        value ^= _arr[(i + 3) % 4];
        value ^= KEY[i];
        
        _arr[i % 4] ^= encrypt_dword_3(value);
    }
    
    for (int i = 0; i < 4; i++) {
        array[i] = rendian(_arr[3 - i]);
    }
}

void encrypt_dword_array_1(unsigned int * array)
{
    for (int i = 0; i < 4; i++) {
        unsigned int value = rint(array[i]);
        value ^= rint(array[(i + 1) % 4]);
        value ^= rint(array[(i + 2) % 4]);
        value ^= rint(array[(i + 3) % 4]);
        
        array[i] = value;
    }
}

void encrypt_sub_bytes(unsigned char * data) 
{
    encrypt_dword_array_0((unsigned int *)data);
    
    encrypt_dword_array_1((unsigned int *)data);
}

unsigned char r4bit(unsigned char c)
{
    return ((c & 0xF0) >> 4) | ((c & 0x0F) << 4);
}

unsigned char * cal_mas_0(const char * as)
{
    unsigned char * mas = (unsigned char *)malloc(27);
    memset(mas, 0x00, 27);
    mas[0] = 1;
    *(unsigned short *)(mas + 1) = 0x8760;
    *(unsigned short *)(mas + 3) = 0x0220;
    
    memcpy(mas + 5, as, 22);
    
    for (int i = 0; i < 27; i++) {
        mas[i] = rbit(mas[i]);
    }
    
    //unsigned char * mas_copy = (unsigned char *)malloc(27);
    unsigned char mas_copy[28];
    memcpy(mas_copy, mas, 27);
    
    for (int i = 0; i < 17; i++) {
        mas[i] = mas_copy[1 + 16 - i];
    }
    
    encrypt_sub_bytes(mas + 1);
    
    for (int i = 17; i < 27; i++) {
        mas[i] = mas_copy[17 + 26 - i];
    }
    
    return mas;
}

const char * cal_mas_1(const unsigned char * mas)
{
    char * mas_str = (char *)malloc(27 * 2 + 1);
    //unsigned char mas_str[27 * 2 + 1];
    memset(mas_str, 0x00, 27 * 2 + 1);
    
    dump_mem2str(mas_str, mas, 27);
    
    return mas_str;
}

const char * cal_mas(const char * as) 
{
    const unsigned char * mas_0 = cal_mas_0(as);
    const char * mas_1 = cal_mas_1(mas_0);
    free((void *)mas_0);
    
    return mas_1;
}

int main(int argc, char * argv[])
{
    unsigned char n1 = 1, n2 = 2, n3 = 0xF0, n4 = 0x5C;
    printf("%02x -> %02x\n", n1, rbit(n1));
    printf("%02x -> %02x\n", n2, rbit(n2));
    printf("%02x -> %02x\n", n3, rbit(n3));
    printf("%02x -> %02x\n", n4, rbit(n4));
    
    
    if (!init) {
        load("ARRAY.dat", ARRAY);
        load("KEY.dat", KEY);
        init = 1;
    }
    
    const char * mas = cal_mas("asbce");
    
    return 0;
}
