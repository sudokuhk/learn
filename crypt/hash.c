/*************************************************************************
 > File Name: hash.c
 > Author: sudoku.huang
 > Mail: sudoku.huang@gmail.com
 > Created Time: Tue 27 Jan 2018 12:16:40 PM CST
 ************************************************************************/

unsigned int SDBMHash(char *str)
{
    unsigned int hash = 0;
    
    while (*str)
    {
        // equivalent to: hash = 65599*hash + (*str++);
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
    }
    
    return (hash & 0x7FFFFFFF);
}

// RS Hash
unsigned int RSHash(char *str)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    
    while (*str)
    {
        hash = hash * a + (*str++);
        a *= b;
    }
    
    return (hash & 0x7FFFFFFF);
}

// JS Hash
unsigned int JSHash(char *str)
{
    unsigned int hash = 1315423911;
    
    while (*str)
    {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }
    
    return (hash & 0x7FFFFFFF);
}

// P. J. Weinberger Hash
unsigned int PJWHash(char *str)
{
    unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
    unsigned int ThreeQuarters  = (unsigned int)((BitsInUnignedInt  * 3) / 4);
    unsigned int OneEighth = (unsigned int)(BitsInUnignedInt / 8);
    unsigned int HighBits = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt
                                                           
                                                           - OneEighth);
    unsigned int hash   = 0;
    unsigned int test   = 0;
    
    while (*str)
    {
        hash = (hash << OneEighth) + (*str++);
        if ((test = hash & HighBits) != 0)
        {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }
    
    return (hash & 0x7FFFFFFF);
}

// ELF Hash
unsigned int ELFHash(char *str)
{
    unsigned int hash = 0;
    unsigned int x  = 0;
    
    while (*str)
    {
        hash = (hash << 4) + (*str++);
        if ((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }
    
    return (hash & 0x7FFFFFFF);
}

// BKDR Hash
//unsigned int BKDRHash(const char * str)
unsigned int hash(const char * str, int size)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;
    
    while (size -- > 0) {
        hash = hash * seed + (*str++);
    }
    
    return (hash & 0x7FFFFFFF);
}

// DJB Hash
unsigned int DJBHash(char *str)
{
    unsigned int hash = 5381;
    
    while (*str)
    {
        hash += (hash << 5) + (*str++);
    }
    
    return (hash & 0x7FFFFFFF);
}

// AP Hash
unsigned int APHash(char *str)
{
    unsigned int hash = 0;
    int i;
    
    for (i=0; *str; i++)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
        }
    }
    
    return (hash & 0x7FFFFFFF);
}
