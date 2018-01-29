/*************************************************************************
  > File Name: uuid.c
  > Author: sudoku.huang
  > Mail: sudoku.huang@gmail.com 
  > Created Time: Tue 14 Nov 2017 04:41:07 PM CST
 ************************************************************************/

#include<stdio.h>

#include <uuid/uuid.h>  

int main()  
{  
    uuid_t uu;  
    int i;  
    uuid_generate( uu );  

    for(i=0;i<16;i++)  
    {  
        printf("%02X-",uu[i]);  
    }  
    printf("\n");  
    return 0;  
}
