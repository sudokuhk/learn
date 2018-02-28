/*************************************************************************
 > File Name: sha256.c
 > Author: sudoku.huang
 > Mail: sudoku.huang@gmail.com
 > Created Time: Fri Feb  9 17:19:36 2018
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char   uint8_t;
typedef unsigned int    uint32_t;

typedef struct
{
    uint32_t    buf[16];
    uint32_t    hash[8];
    uint32_t    len[2];
} sha256_context;

static const uint32_t K[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define RL(x, n)            (((x) << (n)) | ((x) >> (32 - n)))
#define RR(x, n)            (((x) >> (n)) | ((x) << (32 - n)))
#define RS(x, n)            ((x) >> (n))

#define Ch(x, y, z)         (((x) & (y)) ^ ((~x) & (z)))
#define Maj(x, y, z)        (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define S0(x)               (RR((x),  2) ^ RR((x), 13) ^ RR((x), 22))
#define S1(x)               (RR((x),  6) ^ RR((x), 11) ^ RR((x), 25))
#define G0(x)               (RR((x),  7) ^ RR((x), 18) ^ RS((x), 3))
#define G1(x)               (RR((x), 17) ^ RR((x), 19) ^ RS((x), 10))

static void bswapw(uint32_t * p, uint32_t i)
{
    while (i --) {
        p[i] = (RR(p[i], 24) & 0x00FF00FF) | (RR(p[i], 8) & 0xFF00FF00);
    }
}

static void sha256_calc(sha256_context * ctx)
{
    int i, t1, t2, j;
    uint32_t w[64] = {0};
    uint32_t h[8];
    
    for (i = 0; i < 8; i++) {
        h[i] = ctx->hash[i];
    }
    
    for (i = 0; i < 16; i++) {
        w[i] = ctx->buf[i];
    }

    for (i = 16; i < 64; i++) {
        w[i] = G1(w[i - 2]) + w[i - 7] + G0(w[i - 15]) + w[i - 16];
    }
    
    for (i = 0; i < 64; i++) {
        t1 = h[7] + S1(h[4]) + Ch(h[4], h[5], h[6]) + K[i] + w[i];
        t2 = S0(h[0]) + Maj(h[0], h[1], h[2]);
        
        h[7] = h[6];
        h[6] = h[5];
        h[5] = h[4];
        h[4] = h[3] + t1;
        h[3] = h[2];
        h[2] = h[1];
        h[1] = h[0];
        h[0] = t1 + t2;
    }
    
    for (i = 0; i < 8; i++) {
        ctx->hash[i] = ctx->hash[i] + h[i];
        //ctx->hash[i] = (ctx->hash[i] + h[i]) & 0xFFFFFFFF;
    }
}

static void sha256_init(sha256_context * ctx)
{
    memset(ctx, 0x00, sizeof(sha256_context));
    
    ctx->hash[0] = 0x6a09e667;
    ctx->hash[1] = 0xbb67ae85;
    ctx->hash[2] = 0x3c6ef372;
    ctx->hash[3] = 0xa54ff53a;
    ctx->hash[4] = 0x510e527f;
    ctx->hash[5] = 0x9b05688c;
    ctx->hash[6] = 0x1f83d9ab;
    ctx->hash[7] = 0x5be0cd19;
}

static void sha256_update(sha256_context * ctx, const char * bindata, int sz)
{
    uint32_t i = ctx->len[0] & 63;
    uint32_t l, j, k;
    
    ctx->len[0] += sz;
    
    //if needed????
    if (ctx->len[0] < sz) {
        ctx->len[1] ++;
    }
    
    for (j = 0, l = 64 - i; sz >= l; j += l, sz -= l, l = 64, i = 0) {
        memcpy((char *)ctx->buf + i, bindata + j, l);
        bswapw(ctx->buf, 16);
        sha256_calc(ctx);
    }
    
    //copy remaining bytes.
    memcpy((char *)ctx->buf, bindata + j, sz);
}

void show(sha256_context * ctx)
{
    char digest[64] = {0}, hex[1024];
    int off = 0, i;
    
    for (i = 0; i < 32; i++) {
        digest[i] = (uint8_t)(ctx->hash[i >> 2] >> ((~i & 3) << 3));
    }
    
    for (i = 0; i < 32; i++) {
        off += sprintf(hex + off, "%02x", (unsigned char)digest[i]);
    }
    printf("%s\n", hex);
}

static void sha256_done(sha256_context * ctx, char * digest)
{
    uint32_t i = ctx->len[0] & 63, j = i;
    uint8_t * p = (uint8_t *)ctx->buf;
    uint32_t bits = i << 3;
    
    p[j] = 0x80;
    for (j ++ /*skip 0x80*/; j < 64; j++) {
        p[j] = 0;
    }
    
    if (i >= 56) {
        bswapw(ctx->buf, 16);
        sha256_calc(ctx);
        i = 0;
        
        memset(ctx->buf, 0x00, 64);
    }
    
    p[63] = ((ctx->len[0] << 3) >> 0) & 0xFF;
    p[62] = ((ctx->len[0] << 3) >> 8) & 0xFF;;
    p[61] = ((ctx->len[0] << 3) >> 16) & 0xFF;
    p[60] = ((ctx->len[0] << 3) >> 24) & 0xFF;
    
    bswapw(ctx->buf, 16);
    sha256_calc(ctx);
    
    for (i = 0; i < 32; i++) {
        digest[i] = (uint8_t)(ctx->hash[i >> 2] >> ((~i & 3) << 3));
    }
}

static void sha256(const char * bindata, int size, char * digest)
{
    sha256_context ctx;
    
    sha256_init(&ctx);
    sha256_update(&ctx, bindata, size);
    sha256_done(&ctx, digest);
}

int main(int argc, char * argv[])
{
    /*
     * refer: https://1024tools.com/hash
     * 123456 -> 8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92
     */
    char input[1024] = "123456";
    char digest[64] = {0};
    int i, off = 0;
    char hex[64] = {0};
    int j;
    
    sha256(input, strlen(input), digest);
    
    for (i = 0; i < 32; i++) {
        off += sprintf(hex + off, "%02x", (unsigned char)digest[i]);
    }
    input[6] = 0;
    printf("%s -> %s\n", input, hex);
    
    return 0;
}
