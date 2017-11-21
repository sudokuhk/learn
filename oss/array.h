//
//  array.h
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <unistd.h>

typedef struct array
{
    size_t size;
    size_t capacity;
    
    void * data;
} array;

array * create_array(size_t capacity);

void destroy_array(array ** pparray);

int push_array(array * arr, void * data, size_t n);

#endif /* __ARRAY_H__ */
