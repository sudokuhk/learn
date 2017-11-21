//
//  rbtree.h
//  Learn
//
//  Created by huangkun on 3/11/17.
//
//

#ifndef __RBTREE_H__
#define __RBTREE_H__

#include <stdio.h>

typedef struct rb_node
{
    struct rb_node * parent;
    struct rb_node * left;
    struct rb_node * right;
    
    int color;
};
#endif /* __RBTREE_H__ */
