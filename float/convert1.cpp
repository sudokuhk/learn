/*************************************************************************
    > File Name: convert1.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed May 30 17:51:19 2018
 ************************************************************************/

#include <stdio.h>
#include <string.h>

void SwapOrder(void* p, unsigned nBytes)
{
    unsigned char * ptr = (unsigned char *)p;
    unsigned char t;
    unsigned n;
    for(n = 0; n < nBytes>>1; n++)
    {
        t = ptr[n];
        ptr[n] = ptr[nBytes - n - 1];
        ptr[nBytes - n - 1] = t;
    }
}

template<class T, bool b_swap, bool b_signed, bool b_pad> class sucks_v2
{
public:
    inline static void DoFixedpointConvert(const void* source, unsigned bps, unsigned count, float* buffer)
    {
        const char * src = (const char *) source;
        unsigned bytes = bps>>3;
        unsigned n;
        T max = ((T)1)<<(bps-1);
        
        T negmask = - max;
        
        double div = 1.0 / (double)(1<<(bps-1));
        for(n = 0; n < count; n++)
        {
            T temp;
            if (b_pad)
            {
                temp = 0;
                memcpy(&temp, src, bytes);
            }
            else
            {
                temp = *reinterpret_cast<const T*>(src);
            }
            
            if (b_swap)
                SwapOrder(&temp,bytes);
            
            if (!b_signed) temp ^= max;
            
            if (b_pad)
            {
                if (temp & max) temp |= negmask;
            }
            
            if (b_pad)
                src += bytes;
            else
                src += sizeof(T);
            
            buffer[n] = (float)((double)temp * div);
        }
    }
};

template <class T,bool b_pad> class sucks
{
public:
    inline static void DoFixedpointConvert(bool b_swap, bool b_signed, const void* source, unsigned bps,
                                           unsigned count, float* buffer)
    {
        if (sizeof(T) == 1)
        {
            if (b_signed)
            {
                sucks_v2<T, false, true, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
            }
            else
            {
                sucks_v2<T, false, false, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
            }
        }
        else if (b_swap)
        {
            if (b_signed)
            {
                sucks_v2<T, true, true, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
            }
            else
            {
                sucks_v2<T, true, false, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
            }
        }
        else
        {
            if (b_signed)
            {
                sucks_v2<T, false, true, b_pad>::DoFixedpointConvert(source,bps,count,buffer);
            }
            else
            {
                sucks_v2<T, false, false, b_pad>::DoFixedpointConvert(source,bps,count,buffer);
            }
        }
    }
};

enum
{
    FLAG_LITTLE_ENDIAN = 1,
    FLAG_BIG_ENDIAN = 2,
    FLAG_SIGNED = 4,
    FLAG_UNSIGNED = 8,
};

inline bool MachineIsBigEndian()
{
    static bool bMachineIsBigEndian = false;
    static bool bTestMachineIsBigEndian = false;
    
    if (!bTestMachineIsBigEndian)
    {
        unsigned char temp[4];
        *(unsigned int *)temp = 0xDEADBEEF;
        bTestMachineIsBigEndian = true;
        bMachineIsBigEndian = temp[0]==0xDE;
        return bMachineIsBigEndian;
    }
    else
    {
        return bMachineIsBigEndian;
    }
}

bool Data2FixedPoint(void * src, int size, void * dst, int bits, int flag)
{
    int dealcount = size / (bits / 8);

    bool bSigned;
    bool bNeedSwap;
    
    if (flag > 0)
    {
        bSigned = !!(flag & FLAG_SIGNED);
        bNeedSwap = !!(flag & FLAG_BIG_ENDIAN);
        if (MachineIsBigEndian())
        {
            bNeedSwap = !bNeedSwap;
        }
    }
    else
    {
        bSigned = bits > 8;
        bNeedSwap = false;
    }
    
    float* pBuffer = (float *)dst;
    switch(bits)
    {
        case 8:
            sucks<char, false>::DoFixedpointConvert(bNeedSwap, bSigned, src, bits, dealcount, pBuffer);
            break;
        case 16:
            sucks<short, false>::DoFixedpointConvert(bNeedSwap, bSigned, src, bits, dealcount, pBuffer);
            break;
        case 24:
            sucks<long, true>::DoFixedpointConvert(bNeedSwap, bSigned, src, bits, dealcount, pBuffer);
            break;
        case 32:
            sucks<long, false>::DoFixedpointConvert(bNeedSwap, bSigned, src, bits, dealcount, pBuffer);
            break;
        case 40:
        case 48:
        case 56:
        case 64:
        default:
            break;
    }
    
    return true;
}

int ConvertFloatTo16Bit(float * input, char * pData, int nDataSize)
{
    if (nDataSize <= 0)
    {
        return -1;
    }
    float* pSrc = (float*)input;
    short* pDst = (short*)pData;
    union {float f; int i;} u;
    int processCount = nDataSize / 2;
    for (int i = 0; i < processCount; i++)
    {
        u.f = float(*pSrc + 384.0);
        if (u.i > 0x43c07fff)
        {
            *pDst = 32767;
        }
        else if (u.i < 0x43bf8000)
        {
            *pDst = -32768;
        }
        else
        {
            *pDst = short(u.i - 0x43c00000);
        }
        pSrc++;
        pDst++;
    }
    return nDataSize;
}

int main(int argc, char * argv[])
{
    unsigned char input[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    float output[10] = {0};
    char out2[10] = {0};
    
    for (int i = 0; i < 10; i++) {
        printf("%u ", input[i]);
    }
    printf("\n");
    
    Data2FixedPoint(input, 10, output, 16, FLAG_SIGNED);
    for (int i = 0; i < 10; i++) {
        printf("%.4f ", output[i]);
    }
    printf("\n");
    
    ConvertFloatTo16Bit(output, out2, 10);
    for (int i = 0; i < 10; i++) {
        printf("%u ", (unsigned char )out2[i]);
    }
    printf("\n");
    
    return 0;
}
