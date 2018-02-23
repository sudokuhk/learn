#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

enum enHeaderType
{
    enUnknownHeader = -1,
    enPSHeader,         //0xBA
    enSYSHeader,        //0xBB
    enProgramStreamMap, //0xBC
    enPrivate1Header,   //0xBD
    enPaddingStream,    //0xBE
    enPrivate2Header,   //0xBF
    
    enPESAudio,         //0xC0
    enPESVideo,         //0xE0
    
    enECMStream,        //0xF0
    enEMMStream,        //0xF1
    enDSMCCStream,      //0xF2
    enIEC_13522_Stream, //0xF3
    enITU_T_221_A,      //0xF4
    enITU_T_221_B,      //0xF5
    enITU_T_221_C,      //0xF6
    enITU_T_221_D,      //0xF7
    enITU_T_221_E,      //0xF8
    enAncillaryStream,  //0xF9
    enProgramStreamDirectory,   //0xFF
};

enHeaderType get_start_code(int fd)
{
    uint8_t buf[4];
    int n = 0;
    if ((n = read(fd, buf, 4)) != 4) {
        printf("get_start_code:read no such data:%d\n", n);
        return enUnknownHeader;
    }
    
    printf("get_start_code:%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01) {
        return enUnknownHeader;
    }
    
    lseek(fd, -4, SEEK_CUR);
    switch (buf[3]) { 
        case 0xBA:      //PS Header
            return enPSHeader;
        case 0xBB:      
            return enSYSHeader; 
        case 0xBC:
            return enProgramStreamMap;
        case 0xBD:
            return enPrivate1Header;
        case 0xBE:
            return enPaddingStream;   //0xBE
        case 0xBF:
            return enPrivate2Header;   //0xBF
        case 0xF0:
            return enECMStream;        //0xF0
        case 0xF1:
            return enEMMStream;        //0xF1
        case 0xF2:
            return enDSMCCStream;      //0xF2
        case 0xF3:
            return enIEC_13522_Stream; //0xF3
        case 0xF4:
            return enITU_T_221_A;      //0xF4
        case 0xF5:
            return enITU_T_221_B;      //0xF5
        case 0xF6:
            return enITU_T_221_C;      //0xF6
        case 0xF7:
            return enITU_T_221_D;      //0xF7
        case 0xF8:
            return enITU_T_221_E;      //0xF8
        case 0xF9:
            return enAncillaryStream;  //0xF9
        case 0xE0:
            return enPESVideo;
        case 0xC0:
            return enPESAudio;
    }
    
    return enUnknownHeader;
}

bool check_packet_start_code_prefix(int fd)
{
    uint8_t buf[4];
    if (read(fd, buf, 3) != 3) {
        return false;
    }
    
    printf("chk_start_code:%02x %02x %02x\n", buf[0], buf[1], buf[2]);
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01) {
        return false;
    }
    
    lseek(fd, -3, SEEK_CUR);
    return true;
}

void system_header(int fd)
{
    uint8_t buf[100];
    if (read(fd, buf, 6) != 6) {
        printf("read error for pack_header<%s>\n", strerror(errno));
        return;
    }
    
    uint32_t header_length = (buf[4] << 8) | buf[5];
    if (header_length > 0) {
        uint8_t * data = (uint8_t *)malloc(header_length);
        if (read(fd, data, header_length) != header_length) {
            printf("read error for system_header<%s>\n", strerror(errno));
            free(data);
            return;
        }
        free(data);
    }
    printf("read %d bytes system header\n", header_length);
}

void pack_header(int fd)
{
    uint8_t buf[14 + 7];
    
    if (read(fd, buf, 14) != 14) {
        printf("read error for pack_header<%s>\n", strerror(errno));
        return;
    }
    
    if (buf[3] != 0xBA) {
        printf("invalid pack start code:%02x\n", buf[3]);
        return;
    }
    printf("pack_header\n");
    
    {
        uint64_t system_clock_reference_base = 0;
        uint64_t system_clock_reference_extension = 0;
        
        if ((buf[4] >> 2) & 0x01) {
            system_clock_reference_base |= ((buf[0] >> 3) & 0x07);
            system_clock_reference_base <<= 30;
        }
        
        if ((buf[6] >> 2) & 0x01) {
            uint32_t t = ((buf[4] & 0x3) << 13) | (buf[5] << 5) | ((buf[6] >> 3) & 0x1F);
            t <<= 15;
            system_clock_reference_base |= t;
        }
        
        if ((buf[8] >> 2) & 0x01) {
            uint32_t t = ((buf[6] & 0x3) << 13) | (buf[7] << 5) | ((buf[8] >> 3) & 0x1F);                
            system_clock_reference_base |= t;
        }
        
        if (buf[9] & 0x01) {
            system_clock_reference_extension = ((buf[8] & 0x3) << 7) | (buf[9] >> 1);
        }
        printf("system_clock_reference_base = %llu\n", system_clock_reference_base);
        printf("system_clock_reference_extension = %llu\n", system_clock_reference_extension);
    }
    
    uint8_t pack_stuffing_length = buf[13] & 0x07;
    printf("pack_stuffing_length:%d\n", pack_stuffing_length);
    if (pack_stuffing_length > 0) {
        printf("read pack_stuffing_length:%d\n", pack_stuffing_length);
        if (read(fd, buf, pack_stuffing_length) != pack_stuffing_length) {
            printf("read error for pack_stuffing_length:%d\n", 
                pack_stuffing_length);
            return;
        }
    }
    
    if (get_start_code(fd) == enSYSHeader) {
        system_header(fd);
    }
}

void program_stream_map(int fd)
{
    uint8_t buf[6];
    if (read(fd, buf, 6) != 6) {
        printf("read error for program_stream_map<%s>\n", strerror(errno));
        return;
    }
    
    uint32_t header_length = (buf[4] << 8) | buf[5];
    if (header_length > 0) {
        uint8_t * data = (uint8_t *)malloc(header_length);
        if (read(fd, data, header_length) != header_length) {
            printf("read error for program_stream_map<%s>\n", strerror(errno));
            free(data);
            return;
        }
        free(data);
    }
    printf("read %d bytes system program_stream_map\n", header_length);
}

void skip_pes(int fd)
{
    uint8_t buf[6];
    if (read(fd, buf, 6) != 6) {
        printf("read error for skip_pes<%s>\n", strerror(errno));
        return;
    }
    
    uint32_t header_length = (buf[4] << 8) | buf[5];
    if (header_length > 0) {
        uint8_t * data = (uint8_t *)malloc(header_length);
        if (read(fd, data, header_length) != header_length) {
            printf("read error for skip_pes<%s>\n", strerror(errno));
            free(data);
            return;
        }
        free(data);
    }
    printf("read %d bytes system skip_pes\n", header_length);
}

static int h264 = -1;
void pes_video(int fd) 
{
    uint8_t buf[9];
    if (read(fd, buf, 9) != 9) {
        printf("read error for pes_video<%s>\n", strerror(errno));
        return;
    }
    
    uint32_t pes_packet_length = (buf[4] << 8) | buf[5];
    uint8_t data_alignment_indicator = (buf[6] >> 2) & 0x01;
    uint8_t pts_dts_flag = (buf[7] >> 6) & 0x03;
    uint8_t pes_header_data_length = buf[8];
    uint64_t pts = 0;
    
    printf("pes video, length:%d, alignment:%d, pts_dts_flag:%d, pes_header:%d\n",
        pes_packet_length, data_alignment_indicator,
        pts_dts_flag, pes_header_data_length);
       
    if (pes_header_data_length > 0) {
        uint8_t * data = (uint8_t *)malloc(pes_header_data_length);
        if (read(fd, data, pes_header_data_length) != pes_header_data_length) {
            printf("read pes_header_data_length error:%s\n", strerror(errno));
        }
        
        if (pts_dts_flag == 2) {
            
            printf("0010 -> %x\n", (data[0] >> 4) & 0xF);
            if (data[0] & 0x01) {
                pts |= ((data[0] >> 1) & 0x07);
                pts <<= 30;
            }
            
            if (data[2] & 0x01) {
                uint32_t t = ((data[1] << 7) | ((data[2] & 0x7F) >> 1));
                t <<= 15;
                pts |= t;
            }
            
            if (data[4] & 0x01) {
                uint32_t t = ((data[3] << 7) | ((data[4] & 0x7F) >> 1));                
                pts |= t;
            }
        }
        pts /= 90;
        printf("pts:%llu\n", pts);
        
        free(data);
    }
    
    if (pes_packet_length > 0) {
        uint32_t payload = pes_packet_length - 3 - pes_header_data_length;
        printf("pes video, payload:%d\n", payload);
        
        if (payload > 0) {
            uint8_t * data = (uint8_t *)malloc(payload);
            if (read(fd, data, payload) != payload) {
                printf("read payload error:%s\n", strerror(errno));
            }
            
            if (h264 < 0) {
                h264 = open("h264.bin", O_CREAT | O_WRONLY | O_TRUNC, 0666);
            }
            write(h264, data, payload);
            
            free(data);
        }
        
    }
}

void pes_packet(int fd)
{
    enHeaderType type = get_start_code(fd);
    if (type == enProgramStreamMap) {
        program_stream_map(fd);
    } else if (type == enUnknownHeader) {
        printf("unknown...\n");
        exit(0);
    } else if (enPSHeader == type) {
        printf("get program header, reloop\n");
    } else if (enPESVideo == type) {
        pes_video(fd);
    } else {
        printf("skip pes:%d\n", type);
        skip_pes(fd);
    }
}

void pack(int fd)
{
    printf("pack..\n");
    pack_header(fd);
    
    while (get_start_code(fd) != enPSHeader) {
        pes_packet(fd);
    }
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
    
    char buf[100];
    int nread = 0, nremain = 0;
    read(fd, buf, 0x28); //skip file header.
    
    do {
        pack(fd);
    } while (get_start_code(fd) == enPSHeader);
    
    close(fd);
    if (h264 > 0) {
        close (h264);
    }
    
    return 0;
}