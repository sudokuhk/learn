/*************************************************************************
    > File Name: ftp.c
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Sat Aug  4 11:37:24 2018
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

int ftp(const char * ip, int port,
        const char * username, const char * passwd);

static int ftp_connect(const char * ip, int port);
static int ftp_checkresp(int fd, char exp, char * buf, int len);
static int ftp_sendcmd(int fd, const char * cmd);
static int ftp_login(int fd, const char * username, const char * passwd);
static int ftp_passive(int fd);
static int ftp_binarymode(int fd);
static int ftp_list(int fd, const char * remotepath);
static int ftp_upload(int fd, const char * localfile, const char * remotepath);
static int ftp_download(int fd, const char * localfile, const char * remotepath);

int ftp(const char * ip, int port,
        const char * username, const char * passwd)
{
    int ret = 0;
    int sockctl = -1;
    
    printf("step 1. connect\n");
    if ((sockctl = ftp_connect(ip, port)) < 0) {
        printf("connect ftp:%s:%d failed\n", ip, port);
        return -1;
    }
    
    printf("step 1. login\n");
    if (ftp_login(sockctl, username, passwd) < 0) {
        printf("login failed\n");
        return -1;
    }
    
    if (ftp_list(sockctl, "/home/ftpadmin") < 0) {
        printf("ftp list:%s failed\n", "/home/ftpadmin");
        return -1;
    }

    if (ftp_upload(sockctl, "./a.out", "/home/ftpadmin/upload.bin") < 0) {
        printf("upload /home/ftpadmin/upload.bin failed\n");
        return -1;
    }
    
    if (ftp_download(sockctl, "download.bin", "/home/ftpadmin/upload.bin") < 0) {
        printf("download /home/ftpadmin/upload.bin failed\n");
        return -1;
    }
    
    return 0;
}

static int ftp_checkresp(int fd, char exp, char * buf, int len)
{
    char bufin[1024] = {0};
    char * pbuf = bufin;
    if (buf != NULL) {
        pbuf = buf;
    } else {
        len = 1024;
    }
    int nread = recv(fd, pbuf, len, 0);
    if (nread < 0 || pbuf[0] != exp) {
        pbuf[nread] = '\0';
        printf("recv[%d]:%s\n", nread, pbuf);
        return -1;
    }
    return 0;
}

static int ftp_sendcmd(int fd, const char * cmd)
{
    return send(fd, cmd, strlen(cmd), 0);
}

static int ftp_connect(const char * ip, int port)
{
    int sockctl = -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    
    printf("create socket:%s:%d for control\n", ip, port);
    sockctl = socket(AF_INET, SOCK_STREAM, 0);
    if (sockctl < 0) {
        printf("open socket:%s:%d error\n", ip, port);
        goto error;
    }
    
    printf("connect to ctrl\n");
    if (connect(sockctl, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("connect ctl error\n");
        goto error;
    }
    
    printf("check ftp response\n");
    if (ftp_checkresp(sockctl, '2', NULL, 0) < 0) {
        printf("check resp '2' failed\n");
        goto error;
    }
    
    return sockctl;
error:
    if (sockctl > 0) {
        close(sockctl);
        sockctl = -1;
    }
    return sockctl;
}

static int ftp_login(int fd, const char * username, const char * passwd)
{
    char cmd[1024];

    printf("send username:%s\n", username);
    sprintf(cmd, "USER %s\r\n", username);
    if (ftp_sendcmd(fd, cmd) < 0) {
        printf("send username error\n");
        return -1;
    }
    if (ftp_checkresp(fd, '3', NULL, 0) < 0) {
        printf("check username response failed\n");
        return -1;
    }
    
    printf("send password:%s\n", passwd);
    sprintf(cmd, "PASS %s\r\n", passwd);
    if (ftp_sendcmd(fd, cmd) < 0) {
        printf("send password error\n");
        return -1;
    }
    if (ftp_checkresp(fd, '2', NULL, 0) < 0) {
        printf("check password response failed\n");
        return -1;
    }
    return 0;
}

static int ftp_passive(int fd)
{
    char resp[1024];
    union {
        struct sockaddr sa;
        struct sockaddr_in in;
    } sin;
    unsigned int v[6];
    int sock = -1;
    int on = 1;
    struct linger lng = { 0, 0 };
    
    printf("set passive\n");
    if (ftp_sendcmd(fd, "PASV\r\n") < 0) {
        printf("set passive error\n");
        return -1;
    }
    printf("check passive resp\n");
    if (ftp_checkresp(fd, '2', resp, 1024) < 0) {
        printf("check passive resp failed\n");
        return -1;
    }
    
    printf("recv passive:%s\n", resp);
    memset(&sin, 0, sizeof(sin));
    sscanf(resp, "%*[^(](%u,%u,%u,%u,%u,%u",
           &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);
    sin.sa.sa_family = AF_INET;
    sin.sa.sa_data[2] = v[2];
    sin.sa.sa_data[3] = v[3];
    sin.sa.sa_data[4] = v[4];
    sin.sa.sa_data[5] = v[5];
    sin.sa.sa_data[0] = v[0];
    sin.sa.sa_data[1] = v[1];
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("create data socket error\n");
        goto error;
    }
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&on, sizeof(on)) < 0) {
        printf("setsockopt SO_REUSEADDR error\n");
        goto error;
    }
    
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER,
                   (const char *)&lng, sizeof(lng)) < 0) {
        printf("setsockopt SO_LINGER error\n");
        goto error;
    }
    
    if (connect(sock, (const struct sockaddr *)&sin, sizeof(sin)) < 0) {
        printf("connect data socket error\n");
        goto error;
    }
    return sock;
error:
    if (sock > 0) {
        close(sock);
        sock = -1;
    }
    return sock;
}

static int ftp_binarymode(int fd)
{
    if (ftp_sendcmd(fd, "TYPE I\r\n") < 0) {
        printf("switch to binary mode error\n");
        return -1;
    }
    if (ftp_checkresp(fd, '2', NULL, 0) < 0) {
        printf("switch to binary mode response error\n");
        return -1;
    }
    return 0;
}

static int ftp_list(int sockctl, const char * remotepath)
{
    char cmd[1024], buf[4096];
    int nread = 0;
    int sockdata = -1;
    
    if ((sockdata = ftp_passive(sockctl)) < 0) {
        printf("enter passive mode error\n");
        return -1;
    }
    
    printf("ftp list:%s\n", remotepath);
    sprintf(cmd, "NLST %s\r\n", remotepath);
    if (ftp_sendcmd(sockctl, cmd) < 0) {
        printf("list error\n");
        return -1;
    }
    if (ftp_checkresp(sockctl, '1', NULL, 0) < 0) {
        printf("check list error\n");
        return -1;
    }
    
    printf("---recv list----\n");
    while ((nread = recv(sockdata, buf, 4095, 0)) > 0) {
        buf[nread] = '\0';
        printf("%s", buf);
    }
    printf("<---\n");
    close(sockdata);
    
    if (ftp_checkresp(sockctl, '2', NULL, 0) < 0) {
        printf("check list response error\n");
        return -1;
    }
    printf("receive list response ok\n");
    
    return 0;
}

static int ftp_upload(int fd, const char * localfile, const char * remotepath)
{
    char cmd[1024], buf[4096];
    int nread = 0;
    int sockdata = -1;
    int file = -1;
    
    if (ftp_binarymode(fd) < 0) {
        printf("stream mode failed\n");
        return -1;
    }
    
    printf("set passive\n");
    if ((sockdata = ftp_passive(fd)) < 0) {
        printf("enter passive mode error\n");
        return -1;
    }
    
    printf("ftp upload:%s\n", remotepath);
    sprintf(cmd, "STOR %s\r\n", remotepath);
    if (ftp_sendcmd(fd, cmd) < 0) {
        printf("STOR error\n");
        return -1;
    }
    if (ftp_checkresp(fd, '1', NULL, 0) < 0) {
        printf("check STOR error\n");
        return -1;
    }
    
    printf("begin upload file:%s\n", localfile);
    file = open(localfile, O_RDONLY);
    if (file < 0) {
        printf("open file:%s error\n", localfile);
        return -1;
    }
    
    while ((nread = read(file, cmd, 1024)) > 0) {
        printf("read file:%d\n", nread);
        if (send(sockdata, cmd, nread, 0) != nread) {
            printf("send error\n");
            break;
        }
    }
    close(file);
    close(sockdata);
    
    if (ftp_checkresp(fd, '2', NULL, 0) < 0) {
        printf("check upload file response error\n");
        return -1;
    }
    printf("upload file done\n");
    return 0;
}

static int ftp_download(int fd, const char * localfile, const char * remotepath)
{
    char cmd[1024], buf[4096];
    int nread = 0;
    int sockdata = -1;
    int file = -1;
    
    if (ftp_binarymode(fd) < 0) {
        printf("stream mode failed\n");
        return -1;
    }
    
    printf("set passive\n");
    if ((sockdata = ftp_passive(fd)) < 0) {
        printf("enter passive mode error\n");
        return -1;
    }
    
    printf("ftp download:%s\n", remotepath);
    sprintf(cmd, "RETR %s\r\n", remotepath);
    if (ftp_sendcmd(fd, cmd) < 0) {
        printf("entr cmd error\n");
        return -1;
    }
    if (ftp_checkresp(fd, '1', NULL, 0) < 0) {
        printf("check entr response error\n");
        return -1;
    }
    
    printf("begin download file:%s\n", localfile);
    file = open(localfile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (file < 0) {
        printf("open file:%s error\n", localfile);
        return -1;
    }
    
    while ((nread = recv(sockdata, cmd, 1024, 0)) > 0) {
        printf("read file:%d\n", nread);
        if (write(file, cmd, nread) != nread) {
            printf("save error\n");
            break;
        }
    }
    close(file);
    close(sockdata);
    
    if (ftp_checkresp(fd, '2', NULL, 0) < 0) {
        printf("check download file response error\n");
        return -1;
    }
    printf("download file done\n");
    return 0;
}

int main(int argc, char * argv[])
{
    ftp("192.168.43.33", 21, "ftpadmin", "huangkun");
    getchar();
    return 0;
}
