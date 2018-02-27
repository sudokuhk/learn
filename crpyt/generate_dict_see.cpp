#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uint8_t;

int main()
{
    uint8_t dict[256], seed[256];
    int seebegin = 32, seeend = 126;
    int size, add, count;
    
    size = 32, add = 0, count = size;
    for (int i = 0; i < size; i++) {
        seed[i] = i;
    }
   
    for (int i = 0; i < count; i++) {
        int idx = rand() % size;
        dict[i + add] = seed[idx] + add;
        seed[idx] = seed[-- size];
    }
    
    size = 95, add = 32, count = size;
    for (int i = 0; i < size; i++) {
        seed[i] = i;
    }

    for (int i = 0; i < count; i++) {
        int idx = rand() % size;
        dict[i + add] = seed[idx] + add;
        seed[idx] = seed[-- size];
    }
    
    size = 256 - 95 - 32, add = 95 + 32, count = size;;
    for (int i = 0; i < size; i++) {
        seed[i] = i;
    }
    
    for (int i = 0; i < count; i++) {
        int idx = rand() % size;
        dict[i + add] = seed[idx] + add;
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
    
    for (int i = 0; i < 95; i++) {
        printf("%c -> %c\n", (char)(i + 32), dict[i + 32]);
    }
    
    //check
    for (int i = 0; i < 256; i++) {
        seed[i] = 0;
    }
    
    for (int i = 0; i < 256; i++) {
        seed[dict[i]] ++;
    }
    
    for (int i = 0; i < 256; i++) {
        if (seed[i] == 1) {
            continue;
        }
        printf("%d error, count:%d\n", i, seed[i]);
    }
    
    const char see[] = "abcdefghijklmnopqrstuvwxyz01234567890-=_+`~[]{}\ |,./<>?\'\"ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*():;";
    char out[100] = {0};
    printf("[%d]%s\n", strlen(see), see);
    for (int i = 0; i < strlen(see); i++) {
        out[i] = (char)dict[(uint8_t)see[i]];
    }
    printf("[%d]%s\n", strlen(out), out);
    
    return 0;
}
