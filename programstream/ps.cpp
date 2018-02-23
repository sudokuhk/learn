#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

bool getPSHeader(int fd)
{
    uint8_t buf[20];
    int nread = read(fd, buf, 14);
    
    if (nread != 14) {
        return false;
    }
    
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01) {
        return false;
    }
    
    if (buf[3] != 0xBA) {
        return false;
    }
    
    uint8_t pack_stuffiing_length = buf[13] & 0x07;
    if (pack_stuffiing_length > 0) {
        nread = read(fd, buf, pack_stuffiing_length);
        if (nread != pack_stuffiing_length) {
            return false;
        }
    }
    
    return true;
}

enum enHeaderType
{
    enUnknownHeader = -1,
    enPESHeader,
    enSYSHeader,
    enSYSMap,
    enPrivate1Header,
    enPrivate2Header,
    enSysTitle,
};

enHeaderType getNextStartCode(int fd)
{
    uint8_t buf[4];
    if (read(fd, buf, 4) != 4) {
        return enUnknownHeader;
    }
    
    printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01) {
        return enUnknownHeader;
    }
    
    lseek(fd, -4, SEEK_CUR);
    switch (buf[3]) {
        case 0xE0:      //PES Header
            return enPESHeader;   
        case 0xBB:      //System Title
            return enSysTitle;   
        case 0xBC:      //PES Header
            return enSYSHeader; 
        case 0xBD:
            return enPrivate1Header;
        default:
            return enUnknownHeader;
    }
    
    return enUnknownHeader;
}

bool skipPES(int fd)
{
    uint8_t buf[6];
    if (read(fd, buf, 6) != 6) {    //read header, header length.
        return false;
    }
    
    uint32_t header_length = (buf[4] << 8) | buf[5];
    if (read(fd, buf, header_length) != header_length) {
        return false;
    }
    
    return true;
}

bool pickPES(int fd)
{
    uint8_t buf[65536]; //length max 2B
    if (read(fd, buf, 9) != 9) {    //read header, header length.
        return false;
    }
    
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01) {
        return false;
    }
    
    if (buf[3] >= 0xC0 && buf[3] <= 0xCF) {
        printf("PES AUDIO\n");
    } else if (buf[3] >= 0xE0 && buf[3] <= 0xEF) {
        printf("PES VIDEO\n");
    }
    
    uint32_t header_length = (buf[4] << 8) | buf[5];
    printf("header length:%d\n", header_length);
    
    uint8_t padding = buf[8];
    printf("PES_header_data_length:%d\n",  padding);
    if (read(fd, buf, padding) != padding) {
        return false;
    }
    
    uint32_t rawlen = header_length - 3 - padding;
    printf("rawlen:%d\n", rawlen);
    
    if (read(fd, buf, rawlen) != rawlen) {
        return false;
    }
    
    return true;
}

bool pickSYSHeader(int fd)
{
    uint8_t buf[65536]; //length max 2B
    
    if (read(fd, buf, 6) != 6) {    //read header, header length.
        return false;
    }
    
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01 || buf[3] != 0xBC) {
        return false;
    }
    
    uint32_t header_length = (buf[4] << 8) | buf[5];
    printf("header length:%d\n", header_length);
    
    //skip header
    if (read(fd, buf, header_length) != header_length) {
        return false;
    }
    
    while (getNextStartCode(fd) == enPESHeader) {
        pickPES(fd);
    }
    return true;
}

int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("invalid arguments, must be 2 or more\n");
        return 0;
    }
    
    const char * filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("open file:%s failed.<%s>\n", filename, strerror(errno));
        return 0;
    }
    
    char buf[10240];
    int nread = 0, nremain = 0;
    
    read(fd, buf, 0x28); //skip file header.
    while (1) {
        
        if (!getPSHeader(fd)) {
            printf("read program stream pack header error.\n");
            break;
        }
        printf("read program stream pack header.\n");
        
        enHeaderType type = getNextStartCode(fd);
        printf("HeaderType:%d\n", type);
        
        bool ok = false;
        switch (type) {
            case enPESHeader:
                ok = pickPES(fd);
                break;
            case enSYSHeader:
                ok = pickSYSHeader(fd);
                break;
            default:
                skipPES(fd);
                break;
        }
    }
    
    close(fd);
    
    return 0;
}