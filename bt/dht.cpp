/*************************************************************************
    > File Name: dht.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thu Jan  3 14:45:07 2019
 ************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h> //struct sockaddr_in
#include <netdb.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

struct {
    const char * host;
    int port;
} BOOTSTRAP_NODES[] = {
    {"router.bittorrent.com", 6881},
    {"dht.transmissionbt.com", 6881},
    {"router.utorrent.com", 6881},
};

int str2ip(const char * szip)
{
    uint32_t ip = 0;
    
    if (szip != NULL) {
        uint32_t a, b, c, d;
        if (sscanf(szip, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            ip = (a << 24) | (b << 16) | (c << 8) | d;
        }
    }
    
    return ip;
}

std::string base32encode(const char * input, int size)
{
    const static char table[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', '2', '3', '4', '5', '6', '7',
    };
    
    int in_bits = size * 8;    
    int out_bytes = in_bits / 5 + (in_bits / 5 == 0 ? 0 : 1);
    char * output = new char[out_bytes + 1];
    output[out_bytes] = '\0';
    
    int bits = 0, i = 0, j = 0;
    unsigned int t_int = 0;
    unsigned char t_char = 0;
    
    while (i < size) {
        t_char = (unsigned char)input[i];
        bits += 8;
        t_int = (t_int << 8) | t_char;
        
        while (bits >= 5) {
            unsigned int t = (t_int >> (bits - 5)) & 0x1F;
            bits -= 5;
            output[j ++] = table[t];
        }
        
        i ++;
    }
    
    if (bits > 0) {
        t_int <<= 5 - bits;
        unsigned int t = t_int & 0x1F;
        output[j ++] = table[t];
    }
    
    //printf("[%s] -> [%s]\n", input, output);
    std::string res(output);
    delete [] output;
    return res;
}

std::string base32decode(const char * input, int size)
{
    unsigned int t = 0, bits = 0;
    int j = 0, out_bytes = size * 5 / 8;
    
    unsigned char * output = new unsigned char[out_bytes + 1];
    output[out_bytes] = '\0';
    
    for (int i = 0; i < size; i++) {
        if ((input[i] <= 'Z' && input[i] >= 'A') ||
            (input[i] <= '7' && input[i] >= '2')) {
            unsigned char v = 0;
            if (input[i] < 'A') {   //digit
                v = input[i] - '2' + 26;
            } else {    //alpha
                v = input[i] - 'A';
            }
            
            t = (t << 5) | v;            
            bits += 5;
            
            while (bits >= 8) {
                output[j ++] = (t >> (bits - 8)) & 0xFF;
                bits -= 8;
            }
        } else {
            break;
        }
    }
    std::string res;
    res.append((char *)output, j);    
    delete [] output;
    
    return res;
}

int main(int argc, char * argv[]) 
{
    //std::string b32_enc = base32encode("I love you!", strlen("I love you!"));
    //std::string b32_dec = base32decode(b32_enc.c_str(), b32_enc.size());
    
    const char * btih = "VC7WY6JQT6O7NYIRDSHKWGDQZISZ2EPX";
    std::string btih_dec = base32decode(btih, strlen(btih));
    for (int i = 0; i < btih_dec.size(); i++) {
        printf("%02x ", (unsigned char)btih_dec[i]);
    }
    printf("\n");
    //return 0;
    
    char ping[] = "d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe";
    //char find_node[] = "d1:ad2:id20:abcdefghij01234567896:target20:VC7WY6JQT6O7NYIRDSHKWGDQZISZ2EPXe1:q9:find_node1:t2:aa1:y1:qe"; 
    std::string find_node;
    find_node.append("d1:ad2:id20:abcdefghij01234567896:target20:");
    find_node.append(btih_dec);
    find_node.append("e1:q9:find_node1:t2:aa1:y1:qe");
    
    std::string get_peers;
    get_peers.append("d1:ad2:id20:abcdefghij01234567899:info_hash20:");
    get_peers.append(btih_dec);
    get_peers.append("e1:q9:get_peers1:t4:tttt1:y1:qe");
    
    const char * request = get_peers.c_str();
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6881);
    addr.sin_addr.s_addr = htonl(str2ip("67.215.246.10"));
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    sendto(sock, request, strlen(request), 0, (const struct sockaddr *)&addr, sizeof(addr));
    
    char buffer[1024];
    
    while (1) {
        int nrec = recv(sock, buffer, 1024, 0);
        buffer[nrec] = '\0';
        printf("buffer:[%d]%s\n", nrec, buffer);
    }
    
    return 0;
}
