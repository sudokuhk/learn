//
//  hmac_sha1.h
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#ifndef __HMAC_SHA1_H__
#define __HMAC_SHA1_H__

#include <unistd.h>

typedef unsigned char   uint8_t;
typedef unsigned int    uint32_t;

void sha1(uint8_t * msg, int msglen, uint8_t * digest);

void hmac_sha1(unsigned char hmac[20],
               const unsigned char * key, int keylen,
               const unsigned char * msg, int msglen);

#endif /* __HMAC_SHA1_H__ */
