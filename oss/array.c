//
//  array.c
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#include "array.h"

#include <stdlib.h>
#include <string.h>

static int expand(array * arr, size_t capacity);

array * create_array(size_t capacity)
{
    array * arr = NULL;
    
    arr = (array *)malloc(sizeof(array));
    if (arr != NULL) {
        arr->size = 0;
        arr->capacity = 0;
        arr->data = NULL;
        
        expand(arr, capacity);
    }
    
    return arr;
}

void destroy_array(array ** pparray)
{
    array * parray = NULL;
    
    if (pparray != NULL) {
        parray = *pparray;
        
        if (parray != NULL) {
            if (parray->data != NULL) {
                free(parray->data);
            }
            free(parray);
        }
        *pparray = NULL;
    }
}

int push_array(array * arr, void * data, size_t n)
{
    int ret = -1;
    size_t remain = 0;
    
    if (arr == NULL) {
        goto out;
    }
    
    if (data != NULL && n > 0) {
        remain = arr->capacity - arr->size;
        if (remain < n && expand(arr, n) < 0) {
            goto out;
        }

        memcpy((char *)arr->data + arr->size, (char *)data, n);
        arr->size += n;
    }
    ret = 0;
    
out:
    return ret;
}

static int expand(array * arr, size_t capacity)
{
    void * p = NULL;
    int ret = -1;
    
    if (arr == NULL || capacity == 0) {
        return -1;
    }
    
    if (arr->data == NULL) {
        p = (void *)malloc(capacity);
        
        if (p != NULL) {
            arr->capacity = capacity;
            arr->data = p;
            
            *((char *)arr->data) = 0; //init
            
            ret = 0;
        }
    } else {
        p = (void *)realloc(arr->data, capacity);
        if (p != NULL) {
            if (arr->size > capacity) {
                arr->size == capacity;
            }
            arr->capacity = capacity;
            arr->data = p;
            
            ret = 0;
        }
    }
    return ret;
}
