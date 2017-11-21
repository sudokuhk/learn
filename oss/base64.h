//
//  base64.h
//  Learn
//
//  Created by huangkun on 3/17/17.
//
//

#ifndef __BASE64_H__
#define __BASE64_H__

#include <stdio.h>

char * base64_encode(const unsigned char *bin, char * base64, int blen);

int base64_decode( const char * base64, unsigned char * bindata);

#endif /* __BASE64_H__ */
