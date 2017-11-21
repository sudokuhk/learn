//
//  oss_sdk.h
//  Learn
//
//  Created by huangkun on 3/18/17.
//
//

#ifndef __OSS_SDK_H__
#define __OSS_SDK_H__

#include <stdio.h>

typedef const char * ccp;
typedef void (*oss_sdk_callback)(int bytes, int total);

int oss_sdk_update(ccp endpoint, ccp bucket, ccp key, ccp secret,
                   ccp local, ccp remote, ccp type, oss_sdk_callback cb);

int oss_sdk_download(ccp endpoint, ccp bucket, ccp key, ccp secret,
                     ccp local, ccp remote, oss_sdk_callback cb);

#endif /* __OSS_SDK_H__ */
