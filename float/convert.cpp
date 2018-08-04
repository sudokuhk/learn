/*************************************************************************
    > File Name: convert.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed May 30 17:34:37 2018
 ************************************************************************/

#include <stdio.h>
#include <string.h>

typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef unsigned long long uint64;

#define RR(x, n)                (((x) >> (n)) | ((x) << (32 - n)))
#define SWAP_BIT_U8(u8)         (u8)
#define SWAP_BIT_U16(u16)       \
    (((((uint16)(u16)) >> 8) & 0xFF) | ((((uint16)(u16)) & 0xFF) << 8))
#define SWAP_BIT_U32(u32)       \
    (RR((uint32)(u32), 24) & 0x00FF00FF) | (RR((uint32)(u32), 8) & 0xFF00FF00)
#define SWAP_BIT_U64(u64)       \
    ((((uint64)(SWAP_BIT_U32((u64) & 0xFFFFFFFF)) << 32) & 0xFFFFFFFF00000000) \
    | SWAP_BIT_U32(((u64) >> 32) & 0xFFFFFFFF))

#define SDK_FLAG_SIGNED         (1 << 0)
#define SDK_FLAG_BIG_ENDIAN     (1 << 1)


bool is_little_endian(void)
{
    static bool check = false;
    static bool little_endian = true;
    
    if (check) {
        return little_endian;
    }
    
    union {
        uint32    u32;
        struct {
            uint8 a;
            uint8 b;
            uint8 c;
            uint8 d;
        } s32;
    } flag;
    
    flag.u32 = 0x00000001;
    check = true;
    little_endian = (flag.s32.a == 0x01);
    return little_endian;
}

template <typename T>
void swap(T & a, T & b)
{
    T t = a;
    a = b;
    b = t;
}

template<typename T>
static void SwapOrder(void * p, int bytes)
{
    
    if (sizeof(T) == bytes) {
        switch (sizeof(T)) {
            case 1:
                break;
            case 2: {
                uint16 * p16 = (uint16 *)p;
                *p16 = SWAP_BIT_U16(*p16);
                break;
            }
            case 4: {
                uint32 * p32 = (uint32 *)p;
                *p32 = SWAP_BIT_U32(*p32);
                break;
            }
            case 8:{
                uint64 * p64 = (uint64 *)p;
                *p64 = SWAP_BIT_U64(*p64);
                break;
            }
        }
    } else {
        uint8 * ptr = (uint8 *)p;
        for(int i = 0; i < bytes / 2; i ++) {
            swap(ptr[i], ptr[bytes - i - 1]);
        }
    }
}

template<typename T, bool swap, bool sign, bool pad>
static void DoFixedpointConvertIn(const void * src, int bits,
                                  int count, float * dst)
{
    const char * pin = (const char *)src;
    int bytes = bits >> 3;
    
    T max = ((T)1) << (bits - 1);
    T min = -max;
    T tmp;
    
    double div = 1.0 / (double(1 << (bits - 1)));
    for (int i = 0; i < count; i++) {
        if (pad) {
            tmp = 0;
            memcpy(&tmp, pin, bytes);
        } else {
            tmp = *reinterpret_cast<const T *>(pin);
        }
        
        if (swap) {
            SwapOrder<T>(&tmp, bytes);
        }
        
        if (!sign) {
            tmp ^= max;
        }
        
        if (pad && (tmp & max)) {
            tmp |= min;
        }
        
        if (pad) {
            pin += bytes;
        } else {
            pin += sizeof(T);
        }
        
        dst[i] = (float)((double)tmp * div);
    }
}

template<typename T, bool pad>
static void DoFixedpointConvert(bool swap, bool sign, const void * src,
                                int bits, int count, float * dst)
{
    if (sizeof(T) == 1) {
        if (sign) {
            DoFixedpointConvertIn<T, false, true, pad>(src, bits, count, dst);
        } else {
            DoFixedpointConvertIn<T, false, false, pad>(src, bits, count, dst);
        }
    } else if (swap) {
        if (sign) {
            DoFixedpointConvertIn<T, true, true, pad>(src, bits, count, dst);
        } else {
            DoFixedpointConvertIn<T, true, false, pad>(src, bits, count, dst);
        }
    } else {
        if (sign) {
            DoFixedpointConvertIn<T, false, true, pad>(src, bits, count, dst);
        } else {
            DoFixedpointConvertIn<T, false, false, pad>(src, bits, count, dst);
        }
    }
}

int Data2FixedPoint(uint8 * src, int size, float * dst, int bits, uint32 flag)
{
    if (src == NULL || dst == NULL || size == 0) {
        return 0;
    }
    
    int count = (size / (bits / 8));
    bool bsign, bswap;
    
    if (flag > 0) {
        bsign = !!(flag & SDK_FLAG_SIGNED);
        bswap = !!(flag & SDK_FLAG_BIG_ENDIAN);
        
        if (!is_little_endian()) {
            bswap = !bswap;
        }
    } else {
        bsign = bits > 8;
        bswap = false;
    }
    
    switch (bits) {
        case 16:
            DoFixedpointConvert<short, false>(bswap, bsign, src, bits, count, dst);
            break;
        default:
            return 0;
    }
    
    return count;
}

int PointTo16Bit(const float * pin, uint8 * pout, int size)
{
    if (size <= 0 || pin == NULL || pout == NULL) {
        return -1;
    }
    
    const float * src = pin;
    short * dst = (short *)pout;
    
    union {
        float f;
        int i;
    } u;
    
    for (int i = 0; i < size; i++) {
        u.f = (float)(*src + 384.0);
        if (u.i > 0x43c07fff) {
            *dst = 32767;
        } else if (u.i < 0x43bf8000) {
            *dst = -32768;
        } else {
            *dst = (short)(u.i - 0x43c00000);
        }
        src ++;
        dst ++;
    }
    return size * 2;
}

int main(int argc, char * argv[])
{
    uint8 input[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    float output[10] = {0};
    uint8 out2[10] = {0};
    
    for (int i = 0; i < 10; i++) {
        printf("%u ", input[i]);
    }
    printf("\n");
    
    Data2FixedPoint(input, 10, output, 16, SDK_FLAG_SIGNED);
    for (int i = 0; i < 10; i++) {
        printf("%.4f ", output[i]);
    }
    printf("\n");
    
    PointTo16Bit(output, out2, 5);
    for (int i = 0; i < 10; i++) {
        printf("%u ", out2[i]);
    }
    printf("\n");
    
    return 0;
}
