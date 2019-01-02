/*************************************************************************
    > File Name: btfile.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed Jan  2 15:14:28 2019
 ************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <string>
#include <vector>

typedef struct
{
    std::string ed2k;
    char filehash[20];
    long long length;
    char md5sum[32];
    std::string name;
    std::string name_utf8;
    long piece_length;
    //std::vector<char[20]> pieces;
} bt_info_t;

typedef struct
{
    long long length;
    char md5sum[32];
    std::string path;
} bt_files_t;

typedef struct
{
    std::string announce;
    std::vector<std::string> announce_list;
    long creation_date;
    std::string comment;
    std::string created_by;
    
    bool b_files;
    bt_info_t info;
    bt_files_t files;
} bt_file_t;

static int parse(char * dat, int size);
static int parse_dictionary(char * dat, int size);
static int parse_integer(char * dat, int size);
static int parse_list(char * dat, int size);
static int parse_string(char * dat, int size);

int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("input bt torrent seed filename\n");
        return -1;
    }
    
    const char * bt_file = argv[1];
    struct stat st;
    if (stat(bt_file, &st) != 0) {
        printf("stat file<%s> error\n", bt_file);
        return -1;
    }
    printf("file length:%d\n", (int)st.st_size);
    
    int fd = open(bt_file, O_RDONLY);
    if (fd < 0) {
        printf("open file<%s> error\n", bt_file);
        return -1;
    }
    
    char * bt_data = new char[st.st_size];
    if (read(fd, bt_data, st.st_size) != st.st_size) {
        printf("read but not all\n");
        delete [] bt_data;
        return -1;
    }
    
    if (parse(bt_data, st.st_size) < 0) {
        printf("parse failed\n");
    } else {
        printf("parse succeed\n");
    }
    delete [] bt_data;
    
    return 0;
}

static int parse(char * dat, int size)
{
    char type = *dat;
    int n = 0;
    switch (type) {
        case 'd':   //dictionary
            n = parse_dictionary(dat, size);
            break;
        case 'i':   //integer
            n = parse_integer(dat, size);
            break;
        case 'l':   //list
            n = parse_list(dat, size);
            break;
        default:    //string
            n = parse_string(dat, size);
            break;
    }
    if (n < 0) {
        return -1;
    }
    return n;
}

#define SKIP(dat, size, n)  {(dat) += n; (size) -= n;}

static bool isdigit(char c)
{
    if (c >= '0' && c <= '9') {
        return true;
    }
    return false;
}

template <typename T>
static int atoi(char * dat, int size, T & value)
{
    int n = 0;
    value = 0;
    while (size > 0 && isdigit(*dat)) {
        value = value * 10 + (*dat) - '0';
        SKIP(dat, size, 1);
        n ++;
    }
    return n;
}

static int parse_dictionary(char * dat, int size)
{
    int n = 1, n1 = 0;
    SKIP(dat, size, 1);    //skip 'd'

loop:
    n1 = parse_string(dat, size);
    if (n1 < 0) {
        return -1;
    }
    n += n1;
    SKIP(dat, size, n1);
    
    n1 = parse(dat, size);
    if (n1 < 0) {
        return -1;
    }
    n += n1;
    SKIP(dat, size, n1);
    
    if (*dat != 'e') {
        goto loop;
    }
    n += 1;
    
    return n;
}

static int parse_integer(char * dat, int size)
{
    int n = 1;
    SKIP(dat, size, 1); //skip 'i'
    long long value = 0;
    
    n = atoi(dat, size, value);
    printf("integer:[%d]%lld\n", n, value);
    if (n < 0) {
        return -1;
    }
    SKIP(dat, size, n);
    if (*dat != 'e') {
        return -1;
    }
    return n + 2;
}

static int parse_list(char * dat, int size)
{
    char * b_dat = dat;
    int n = 1;
loop:
    SKIP(dat, size, 1); //skip 'l'
    n = parse(dat, size);
    if (n < 0) {
        return -1;
    }
    SKIP(dat, size, n);
    if (*dat != 'e') {
        goto loop;
    }
    SKIP(dat, size, 1);
    
    return dat - b_dat;
}

static int parse_string(char * dat, int size)
{
    char * b_dat = dat;
    int nstring = 0;
    int n = atoi(dat, size, nstring);
    if (n <= 0) {
        return -1;
    }
    
    SKIP(dat, size, n);
    if (*dat != ':') {
        return -1;
    }
    SKIP(dat, size, 1);
    
    char * string = new char[nstring + 1];
    memcpy(string, dat, nstring);
    string[nstring] = '\0';
    printf("string:[%d]%s\n", nstring, string);
    SKIP(dat, size, nstring);
    
    return dat - b_dat;
}
