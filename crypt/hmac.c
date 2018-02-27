/*************************************************************************
 > File Name: hmac.c
 > Author: sudoku.huang
 > Mail: sudoku.huang@gmail.com
 > Created Time: Tue 27 Jan 2018 12:16:40 PM CST
 ************************************************************************/

#include <string.h>

void hmac_sha1(unsigned char hmac[20],
               const unsigned char * key, int keylen,
               const unsigned char * msg, int msglen)
{
#define HMAC_B      64
#define DIGEST_L    20
    unsigned char kopad[HMAC_B], kipad[HMAC_B];
    unsigned char digest[DIGEST_L];
    unsigned char in[1024];
    int inlen = 0;
    int i;
    
    if (keylen > HMAC_B) {
        keylen = HMAC_B;
    }
    
    for (i = 0; i < keylen; i++) {
        kopad[i] = key[i] ^ 0x5c;
        kipad[i] = key[i] ^ 0x36;
    }
    
    for ( ; i < HMAC_B; i++) {
        kopad[i] = 0 ^ 0x5c;
        kipad[i] = 0 ^ 0x36;
    }
    
    memcpy((char *)in, (char *)kipad, HMAC_B);
    memcpy((char *)in + HMAC_B, msg, msglen);
    inlen = HMAC_B + msglen;
    
    sha1((const char *)in, inlen, (char *)digest);
    
    memcpy((char *)in, (char *)kopad, HMAC_B);
    memcpy((char *)in + HMAC_B, (char *)digest, DIGEST_L);
    inlen = HMAC_B + DIGEST_L;
    
    sha1((const char *)in, inlen, (char *)hmac);
#undef HMAC_B
#undef DIGEST_L
}
