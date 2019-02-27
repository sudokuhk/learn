/*************************************************************************
    > File Name: my_fft.c
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed Feb 27 10:32:45 2019
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

enum TYPE {
    FFT,
    IFFT,
};

typedef struct complex_t {
    double real;
    double imaginary;
} complex_t;

void do_reverse(int * array, int bits)
{
    int n = 1 << bits, i;    
    for (i = 0; i < n; i ++) {
        array[i] = (array[i >> 1] >> 1) | ((i & 1) << (bits - 1));
    }
}

void do_init_w(complex_t * W, int n)
{
    int i = 0;
    const double PI = acos(-1);
    
    for (i = 0; i < n; i++) {
        //e^(ix) = cos(x) + isin(x)
        W[i].real = cos(PI / i);
        W[i].imaginary = sin(PI / i);
    }
}

void print(int * array, int n)
{
    int i = 0;
    printf("array:");
    for (i = 0; i < n; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

void to_complex(char * sz_num, int n, complex_t * cp)
{
    int i = 0;
    for (i = 0; i < n; i++) {
        cp[i].real = sz_num[n - 1 - i] - '0';
        cp[i].imaginary = 0;
    }
}

void complex_swap(complex_t * l, complex_t * r)
{
    complex_t t = *l;
    *l = *r;
    *r = t;
}

complex_t complex_add(const complex_t * l, const complex_t * r)
{
    complex_t result = {0, 0};
    result.real = (l->real + r->real);
    result.imaginary = (l->imaginary + r->imaginary);
    return result;
}

complex_t complex_sub(const complex_t * l, const complex_t * r)
{
    complex_t result = {0, 0};
    result.real = (l->real - r->real);
    result.imaginary = (l->imaginary - r->imaginary);
    return result;
}

complex_t complex_mul(const complex_t * l, const complex_t * r)
{
    complex_t result = {0, 0};
    result.real = (l->real * r->real) - (l->imaginary * r->imaginary);
    result.imaginary = (l->real * r->imaginary) + (l->imaginary * r->real);
    return result;
}

complex_t complex_div(const complex_t * l, const complex_t * r)
{
    complex_t result = {0, 0};
    double denominator = r->real * r->real + r->imaginary * r->imaginary;
    
    result.real = ((l->real * r->real) + (l->imaginary * r->imaginary)) / denominator;
    result.imaginary = ((l->real * r->imaginary) + (l->imaginary * r->real)) / denominator;
    
    return result;
}

void fft(complex_t * arr, int n, 
         const int * r_arr, const complex_t * W, enum TYPE type)
{
    int i, j, k, step;
    
    for (i = 0; i < n; i++) {
        if (i < r_arr[i]) {
            complex_swap(&arr[i], &arr[r_arr[i]]);
        }
    }
    
//    for (i = 0; i < n; i++) {
//        printf("fft[%d] = {%f, %f}\n", i, arr[i].real, arr[i].imaginary);
//    }
    
    for (step = 1; step < n; step <<= 1) {
        complex_t wn = W[step]; //W^step, unit complex root
//        printf("wn[%d] = {%f, %f}\n", step, wn.real, wn.imaginary);
        if (type == IFFT) {
            wn.imaginary *= -1.0;
        }
        
        for (j = 0; j < n; j += (step << 1)) {
            complex_t wnk = {1, 0};
            
            for (k = j; k < j + step; k++) {
                //F(x) = G(x^2) + w * H(x^2)
                complex_t Gx = arr[k];
                complex_t wHx = complex_mul(&wnk, &arr[k + step]);
//                printf("x(%f, %f), y(%f, %f)\n",
//                       Gx.real, Gx.imaginary,
//                       wHx.real, wHx.imaginary);
                
                arr[k] = complex_add(&Gx, &wHx);
                arr[k + step] = complex_sub(&Gx, &wHx);
                
                wnk = complex_mul(&wnk, &wn);
                
//                printf("a[%d] = (%f, %f), a[%d] = (%f, %f), wnk:(%f, %f)\n", 
//                       k, arr[k].real, arr[k].imaginary, 
//                       k + step, arr[k + step].real, arr[k + step].imaginary,
//                       wnk.real, wnk.imaginary);
            }
        }
    }
    
    if (type == IFFT) {
        for (i = 0; i < n; i++) {
            complex_t _n = {n, 0};
            arr[i] = complex_div(&arr[i], &_n);
        }
    }
}

int main(int argc, char * argv[])
{
    char num1[100] = "123456789";
    char num2[100] = "987654321";
    //123456789 * 987654321 = 121932631112635269
    
    int l_num1 = strlen(num1);
    int l_num2 = strlen(num2);
    int l_total = l_num1 + l_num2 - 1;    
    int bits, n, i;
    int * p_reverse = NULL;
    complex_t * p_num1 = NULL, *p_num2 = NULL, *W = NULL;
    int * result = NULL;
    
    while ((n = (1 << bits)) < l_total) bits ++;    
    
    p_num1 = (complex_t *)malloc(n * sizeof(complex_t));
    p_num2 = (complex_t *)malloc(n * sizeof(complex_t));
    p_reverse = (int *)malloc(n * sizeof(int));
    W = (complex_t *)malloc(n * sizeof(complex_t));
    result = (int *)malloc((n + 1) * sizeof(int));
    
    memset(p_num1, 0, n * sizeof(complex_t));
    memset(p_num2, 0, n * sizeof(complex_t));
    memset(p_reverse, 0, n * sizeof(int));
    memset(result, 0, n * sizeof(int));
    
    do_reverse(p_reverse, bits);
    do_init_w(W, n);
    print(p_reverse, n);
    
    to_complex(num1, l_num1, p_num1);
    fft(p_num1, n, p_reverse, W, FFT);
//    for (i = 0; i < n; i++) {
//        printf("a[%d] = {%f, %f}\n", i, p_num1[i].real, p_num1[i].imaginary);
//    }
    
    to_complex(num2, l_num2, p_num2);
    fft(p_num2, n, p_reverse, W, FFT);
//    for (i = 0; i < n; i++) {
//        printf("b[%d] = {%f, %f}\n", i, p_num2[i].real, p_num2[i].imaginary);
//    }
    
    for (i = 0; i < n; i++) {
        p_num1[i] = complex_mul(&p_num1[i], &p_num2[i]);
    }
    fft(p_num1, n, p_reverse, W, IFFT);
//    for (i = 0; i < n; i++) {
//        printf("result[%d] = {%f, %f}\n", i, p_num1[i].real, p_num1[i].imaginary);
//    }
    
    result[n] = '\0';
    for (i = 0; i< n; i++) {
        result[i] += (int)(p_num1[i].real + 0.5);
        if (result[i] > 9) {
            result[i + 1] += result[i] / 10;
            result[i] %= 10;
        }
        //result += '0';
    }
    
    i = n - 1;
    while (result[i] == 0) i--;
    for (; i >= 0; i--) {
        printf("%d", result[i]);
    }
    printf("\n");
    
    free(p_reverse);
    free(p_num2);
    free(p_num1);
    free(W);
    free(result);
    
    return 0;
}
