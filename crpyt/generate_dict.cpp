#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uint8_t;

int main()
{
    uint8_t dict[256], seed[256];
    
    for (int i = 0; i < 256; i++) {
        seed[i] = i;
    }
    
    int size = 256;
    for (int i = 0; i < 256; i++) {
        int idx = rand() % size;
        dict[i] = seed[idx];
        seed[idx] = seed[-- size];
    }
    
    int line = 0;
    for (int i = 0; i < 256; i++) {
        printf("0x%02x, ", dict[i]);
        
        line ++;
        if (line == 8) {
            printf("\n");
            line = 0;
        }
    }
    
    return 0;
}
