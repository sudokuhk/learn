/*************************************************************************
    > File Name: main.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Fri Feb  9 15:19:38 2018
 ************************************************************************/

#include <stdio.h>
#include <math.h>

size_t power(size_t a, size_t b, size_t mod)
{
    size_t sum = a;
    for (size_t i = 1; i < b; i++) {
        sum *= a;
        sum %= mod;
        //printf("sum:%zd\n", sum);
    }
    return sum;
}

int main(int argc, char * argv[])
{
    size_t p = 61 ;
    size_t q = 53;
    size_t n = p * q;
    size_t seta = (p - 1) * (q - 1);

    printf("p = %zd\n", p);
    printf("q = %zd\n", q);
    printf("n = %zd\n", n);
    printf("seta = %zd\n", seta);

    size_t e = 17;
    size_t re = 0;
    for (size_t i = 2; i < seta; i++) {
        for (size_t j = 2; j < seta; j++) {
            if (i * e - j * seta == 1) {
                printf("i:%zd, j:%zd\n", i, j);
                re = i;
                break;
            }
        }
    }
    printf("e:%zd, e(-1):%zd\n", e, re);

    //encrypt
    size_t m = 65;
    size_t powm = power(m, e, n);
    printf("powm:%zd\n", powm);
    size_t c = powm % n;
    printf("m:%zd, c:%zd\n", m, c);

    // decrypt
    size_t o = power(c, re, n);
    printf("o=%zd, d:%zd\n", o, o % n);

    return 0;
}
