/*************************************************************************
    > File Name: addwavhead.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thu Jun 14 18:13:54 2018
 ************************************************************************/

#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
    if (argc < 3) {
        printf("invalid arguments\n");
        return 0;
    }
    
    struct stat sb;
    if (stat(argv[1], &sb) == -1) {
        printf("stat error\n");
        return 0;
    }
    printf("input file length:%d\n", sb.st_size);
    
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
    
    unsigned char head[44];
    head[0] = 'R';
    head[1] = 'I';
    head[2] = 'F';
    head[3] = 'F';
    
    *(unsigned int *)(head + 4) = sb.st_size + 36;
    
    head[8] = 'W';
    head[9] = 'A';
    head[10] = 'V';
    head[11] = 'E';
    
    head[12] = 'f';
    head[13] = 'm';
    head[14] = 't';
    head[15] = ' ';
    
    *(unsigned int *)(head + 16) = 16;
    *(unsigned short *)(head + 20) = 1;
    *(unsigned short *)(head + 22) = 2;
    *(unsigned int *)(head + 24) = 44100;
    *(unsigned int *)(head + 28) = 1 * 44100 * 16 / 8;
    *(unsigned int *)(head + 32) = 1 * 16 / 8;
    *(unsigned int *)(head + 34) = 16;
    
    head[36] = 'd';
    head[37] = 'a';
    head[38] = 't';
    head[39] = 'a';
    
    *(unsigned int *)(head + 40) = sb.st_size;

    write(ofd, head, 44);

    char buf[1024];
    int nread = 0;
    while ((nread = read(fd, buf, 1024)) > 0) {
        write(ofd, buf, nread);
    }
    
    close(fd);
    close(ofd);
    
    return 0;
}
