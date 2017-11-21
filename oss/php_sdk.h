//
//  php_sdk.h
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#ifndef __PHP_SDK_H__
#define __PHP_SDK_H__

#include "array.h"

int do_php_post_request(const char * host, const char * path, const char * type,
    const char * data, size_t ndata, array * arr);

#endif /* __PHP_SDK_H__ */
