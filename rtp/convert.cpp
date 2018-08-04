/*************************************************************************
    > File Name: convert.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thu Jun 14 18:06:49 2018
 ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
    if (argc < 3) {
        printf("invalid arguments\n");
        return 0;
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("can't open %s]n", argv[1]);
        return  0;
    }
    
    int ofd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (ofd < 0) {
        printf("can't open output file:%s\n", argv[2]);
        return 0;
    }
    
#define CHUNK (12 + 160)
    
    int size = CHUNK;
    char buf[CHUNK];
    int nread = 0;
    int count = 0;
    while ((nread = read(fd, buf, CHUNK)) == CHUNK) {
        write(ofd, buf + 12, CHUNK - 12);
        count ++;
    }
    printf("read result:%d, loop count:%d\n", nread, count);
    
    close(fd);
    close(ofd);
    return 0;
}
