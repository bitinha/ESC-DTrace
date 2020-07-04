#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

//Otimização para aproveitar a localidade espacial ao percorrer as linhas
#define bs 4  //L1d SIZE = 32KiB; int = 4B; ints que cabem na cache = 8192; Assumindo o tamanho de linha da cache como 64B, 16 ints por linha
              // nº de linhas = 512
              
#include "ddt.h"

int encontraMax(unsigned int* __restrict__ out, int N){

    unsigned int max = 0;

    #pragma omp parallel for reduction(max:max)
    for (int i = 0; i < N; i++) {
        if (out[i] > max) {
            max = out[i];
        }
    }

    return max;
}

void dt(unsigned char* img, unsigned int* g, int width, int height){

    int max = width + height;
    double time;
    int x;

    //Passo 1
    time = omp_get_wtime();

    if(DTGET_QUERY_PRIMEIROPASSO_ENABLED())
       DTGET_QUERY_PRIMEIROPASSO();

    // Preencher distancias no 1º bloco
    #pragma omp parallel for 
    for (int y = 0; y < width; y++) {
        if (img[y] == 0) {
            g[y] = 0;
        }
        else {
            g[y] = max;
        }

        for (int x = 1; x < bs; x++) {
            if (img[x * width + y] == 0) {
                g[x * width + y] = 0;
            }
            else {
                g[x * width + y] = 1 + g[(x-1) * width + y];
            }
        }
    }

    for(x = bs; x <= height - bs; x += bs) {
        #pragma omp parallel for
        for (int y = 0; y < width; y++) {

            for (int xx = x; xx < x + bs; xx++) {
                if (img[xx * width + y] == 0) {
                    g[xx * width + y] = 0;
                }
                else {
                    g[xx * width + y] = 1 + g[(xx-1) * width + y];
                }
            }
        }
    }

    //terminar o ultimo bloco
    #pragma omp parallel for
    for (int y = 0; y < width; y++) {

        for (int xx = x; xx < height; xx++) {
            if (img[xx * width + y] == 0) {
                g[xx * width + y] = 0;
            }
            else {
                g[xx * width + y] = 1 + g[(xx-1) * width + y];
            }
        }
    }

    time = omp_get_wtime() - time;
    printf("T1: %f\n", time);
    
    if (DTGET_QUERY_SEGUNDOPASSO_ENABLED())
        DTGET_QUERY_SEGUNDOPASSO();

    //Passo 2
    time = omp_get_wtime();
    
    // Preencher distancias no 1º bloco (bloco de baixo)
    #pragma omp parallel for 
    for (int y = 0; y < width; y++) {

        for (int x = height - 2; x >= height-bs; x--) {
            if (g[(x+1) * width + y] < g[x * width + y]) {
                g[x * width + y] = 1 + g[(x+1) * width + y];
            }
        }
    }

    for (x = height - bs; x >= bs; x -= bs){

        #pragma omp parallel for
        for (int y = 0; y < width; y++) {

            for (int xx = x - 1; xx >= x-bs; xx--) {
                if (g[(xx+1) * width + y] < g[xx * width + y]) {
                    g[xx * width + y] = 1 + g[(xx+1) * width + y];
                }
            }
        }
    }

    #pragma omp parallel for 
    for (int y = 0; y < width; y++) {

        for (int xx = x - 1; xx >= 0; xx--) {
            if (g[(xx+1) * width + y] < g[xx * width + y]) {
                g[xx * width + y] = 1 + g[(xx+1) * width + y];
            }
        }
    }

    time = omp_get_wtime() - time;
    printf("T2: %f\n", time);

    time = omp_get_wtime();
    
    if (DTGET_QUERY_TERCEIROQUARTOPASSO_ENABLED())
        DTGET_QUERY_TERCEIROQUARTOPASSO();

    #pragma omp parallel for
    for (int x = 0; x < height; x++) {

        if (DTGET_QUERY_TERCEIROPASSO_ENABLED())
            DTGET_QUERY_TERCEIROPASSO();
        //Passo 3
        for (int y = 1; y < width; y++) {
            if (g[x * width + y] > g[x * width + (y-1)] + 1) {
                g[x * width + y] = g[x * width + (y-1)] + 1;
            }
        }

        if (DTGET_QUERY_QUARTOPASSO_ENABLED())
            DTGET_QUERY_QUARTOPASSO();
        //Passo 4
        for (int y = width - 2; y >=0 ; y--) {
            if (g[x * width + y] > g[x * width + (y+1)] + 1) {
                g[x * width + y] = g[x * width + (y+1)] + 1;
            }
        }
    }   
    
    if(DTGET_QUERY_FIM_ENABLED())
       DTGET_QUERY_FIM();
    time = omp_get_wtime() - time;
    printf("T3+T4: %f\n", time);
}


void transformaGS(unsigned char* __restrict__ img, unsigned int* __restrict__ out, int N){

    if(DTGET_QUERY_MAXENTRY_ENABLED())
       DTGET_QUERY_MAXENTRY(out,N);
    unsigned int max = encontraMax(out,N);
    if(DTGET_QUERY_MAXRETURN_ENABLED())
       DTGET_QUERY_MAXRETURN(max);
    printf("%d\n", max);
    double div = 255.0/max;
    
    if(DTGET_QUERY_TRANSFORMENTRY_ENABLED())
       DTGET_QUERY_TRANSFORMENTRY();

    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        img[i] = out[i] * div;
    }

    if(DTGET_QUERY_TRANSFORMRETURN_ENABLED())
       DTGET_QUERY_TRANSFORMRETURN();
}


int main(int argc, char const *argv[]){
    int width, height, channels;
    if(argc<2) {
        printf("Indique uma imagem\n");
        exit(2);
    }
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);

    if (img == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

    unsigned int *g = malloc(width * height * sizeof(unsigned int));

    double time = omp_get_wtime();
    dt(img,g,width,height);
    time = omp_get_wtime() - time;
    printf("%f\n", time);

    transformaGS(img, g,  width * height);

    stbi_write_png("output.png", width, height, 1, img, width);
}
