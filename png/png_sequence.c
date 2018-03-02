#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

typedef unsigned char uint8_t;

int main(int argc, char * argv[])
{
    int fd, ofd = -1;
    char * buf = NULL;
    int fsize = 0, off = 0, onesize = 0;
    int count = 0;
    
    if (argc < 2) {
        fprintf(stderr, "input invalid. must input file name.\n");
        fprintf(stderr, "Usage: %s pngsequencefile\n", argv[0]);
        return 0;
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open file<%s> error<%s>\n", argv[1], strerror(errno));
        exit(0);
    }
    
    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    //printf("fsize:%d\n", fsize);
    
    buf = (char *)malloc(fsize);
    if (read(fd, buf, fsize) != fsize) {
        printf("read all into buf error.<%s>\n", strerror(errno));
        exit(0);
    }
    
    while (1) {
        const uint8_t * ptr = (const uint8_t *)buf + off;
        char filename[100];
        onesize = 0;
        
        //check header
        const char magic[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0xa, 0x1a, 0x0a};
        if (fsize < 8 && memcmp(magic, ptr, 8) != 0) {
            fprintf(stderr, "magic not match, error\n");
            exit(-1);
        }
        
        onesize += 8;
        
        //analyse chunk
        while (1) {
            int chunksize = 0;
            int type = 0;
            int crc = 0;
            
            
            if (fsize - onesize < 12) {
                fprintf(stderr, "invalid chunk.\n");
                exit(-2);
            }
            
            chunksize |= (ptr[onesize + 0] << 24)
                        | (ptr[onesize + 1] << 16)
                        | (ptr[onesize + 2] << 8)
                        | (ptr[onesize + 3] << 0);
            
            type = (ptr[onesize + 4] << 24)
                    | (ptr[onesize + 5] << 16)
                    | (ptr[onesize + 6] << 8)
                    | (ptr[onesize + 7] << 0);
            
            //printf("chunksize:%x\n", chunksize);
            if (fsize - 12 < chunksize) {
                printf("chunksize:%x, invalid\n", chunksize);
                exit(-3);
            }
            
            onesize += 12 + chunksize;
            
            if (chunksize == 0) {
                //printf("IEND, size:%d\n", onesize);
                break;
            }
        }
        
        fsize -= onesize;
        off += onesize;
        
        sprintf(filename, "out_%d.png", count ++);
        printf("write to %s, size:%d\n", filename, onesize);
        {
            ofd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
            write(ofd, ptr, onesize);
            close(ofd);
        }
        
        if (fsize == 0) {
            printf("DONE\n");
            break;
        }
    }
    
    free(buf);
    close(fd);
    return 0;
}
