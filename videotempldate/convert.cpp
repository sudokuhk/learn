/*************************************************************************
    > File Name: convert.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Fri 13 Oct 2017 03:27:50 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <3rd/libyuv/libyuv.h>

typedef unsigned char uint8;

int main(int arg, char * argv[])
{
    int mask = open(argv[1], O_RDONLY);
    int sucai = open(argv[2], O_RDONLY);
    int merge = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY);
    int w = 480, h = 480;
    int nyuv = w * h * 3 / 2;
    int nrgba = w * h * 4;

    uint8 * pmask = (uint8 *)malloc(nyuv);
    uint8 * psucai = (uint8 *)malloc(nyuv);
    uint8 * rgba = (uint8 *)malloc(nrgba);

    read(mask, pmask, nyuv);
    read(sucai, psucai, nyuv);
    close(mask);
    close(sucai);

//    libyuv::I420ToRGBA(
    libyuv::I420ToABGR(
            psucai, w, 
            psucai + w * h, w >> 1, 
            psucai + w * h * 5 / 4, w >> 1,
            rgba, w * 4, w, h);

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int off = i * w + j;
            rgba[off * 4 + 3] = pmask[off];
        }
    }

    write(merge, rgba, nrgba);
    close(merge);

    free(pmask);
    free(psucai);
    free(rgba);

    return 0;
}
