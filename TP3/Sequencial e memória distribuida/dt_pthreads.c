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
              
#include <pthread.h>
#include "ddt.h"

int thread_count;  
pthread_barrier_t barrier;
double divisor;


typedef struct thread_data {
    unsigned int *g;
    unsigned char *img;
    long rank;
    int max;
    int width, height;

} thread_data;


void *encontraMax(void* arg){

    thread_data *tdata=(thread_data *)arg;

    long my_rank=tdata->rank;
    unsigned int *g = tdata->g;
    int height = tdata->height;
    int width = tdata->width;

    unsigned int max = 0;


    int i = ((width * height) / thread_count) * my_rank;

    int f = (my_rank != thread_count-1) ? i + (width * height) / thread_count : i + (width * height) / thread_count + (width * height) % thread_count;

    for (; i < f; i++) {
        if (g[i] > max) {
            max = g[i];
        }
    }

    tdata->max = max;

    return NULL;
}

void *dt(void* arg){

    thread_data *tdata=(thread_data *)arg;

    long my_rank=tdata->rank;
    unsigned int *g = tdata->g;
    unsigned char *img = tdata->img;
    int height = tdata->height;
    int width = tdata->width;

    int max = width + height;
    int x;


    int i_height = (height / thread_count) * my_rank;
    int i_width = (width / thread_count) * my_rank;

    int f_height = (my_rank != thread_count-1) ? i_height + height / thread_count : i_height + height / thread_count + height % thread_count;
    int f_width = (my_rank != thread_count-1) ? i_width + width / thread_count : i_width + width / thread_count + width % thread_count;


    //Passo 1


    if(DTGET_QUERY_PRIMEIROPASSO_ENABLED())
       DTGET_QUERY_PRIMEIROPASSO();

    // Preencher distancias no 1º bloco
    for (int y = i_width; y < f_width; y++) {
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
        for (int y = i_width; y < f_width; y++) {

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
    for (int y = i_width; y < f_width; y++) {

        for (int xx = x; xx < height; xx++) {
            if (img[xx * width + y] == 0) {
                g[xx * width + y] = 0;
            }
            else {
                g[xx * width + y] = 1 + g[(xx-1) * width + y];
            }
        }
    }

    //Passo 2
    

    if (DTGET_QUERY_SEGUNDOPASSO_ENABLED())
        DTGET_QUERY_SEGUNDOPASSO();

    // Preencher distancias no 1º bloco (bloco de baixo)
    for (int y = i_width; y < f_width; y++) {

        for (int x = height - 2; x >= height-bs; x--) {
            if (g[(x+1) * width + y] < g[x * width + y]) {
                g[x * width + y] = 1 + g[(x+1) * width + y];
            }
        }
    }

    for (x = height - bs; x >= bs; x -= bs){

        for (int y = i_width; y < f_width; y++) {

            for (int xx = x - 1; xx >= x-bs; xx--) {
                if (g[(xx+1) * width + y] < g[xx * width + y]) {
                    g[xx * width + y] = 1 + g[(xx+1) * width + y];
                }
            }
        }
    }

    for (int y = i_width; y < f_width; y++) {

        for (int xx = x - 1; xx >= 0; xx--) {
            if (g[(xx+1) * width + y] < g[xx * width + y]) {
                g[xx * width + y] = 1 + g[(xx+1) * width + y];
            }
        }
    }

    pthread_barrier_wait (&barrier);


    if (DTGET_QUERY_TERCEIROQUARTOPASSO_ENABLED())
        DTGET_QUERY_TERCEIROQUARTOPASSO();


    for (int x = i_height; x < f_height; x++) {

        //Passo 3
        for (int y = 1; y < width; y++) {
            if (g[x * width + y] > g[x * width + (y-1)] + 1) {
                g[x * width + y] = g[x * width + (y-1)] + 1;
            }
        }

        //Passo 4
        for (int y = width - 2; y >=0 ; y--) {
            if (g[x * width + y] > g[x * width + (y+1)] + 1) {
                g[x * width + y] = g[x * width + (y+1)] + 1;
            }
        }
    }   


    if(DTGET_QUERY_FIM_ENABLED())
       DTGET_QUERY_FIM();

    return NULL;
    
}


void *transformaGS(void* arg){


    thread_data *tdata=(thread_data *)arg;

    long my_rank=tdata->rank;
    unsigned int *g = tdata->g;
    unsigned char *img = tdata->img;
    int height = tdata->height;
    int width = tdata->width;


    int i = ((width * height) / thread_count) * my_rank;

    int f = (my_rank != thread_count-1) ? i + (width * height) / thread_count : i + (width * height) / thread_count + (width * height) % thread_count;

    // #pragma omp parallel for
    for (; i < f; i++) {
        img[i] = g[i] * divisor;
    }

    return NULL;
}


int main(int argc, char const *argv[]){
    
    
    unsigned int *g;
    unsigned char *img;
    int width, height, channels;

    if(argc<2) {
        printf("Indique uma imagem\n");
        exit(2);
    }
    img = stbi_load(argv[1], &width, &height, &channels, 0);

    if (img == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

    g = malloc(width * height * sizeof(unsigned int));


    long       thread;  /* Use long in case of a 64-bit system */
    pthread_t* thread_handles; 

    thread_count = strtol(argv[2], NULL, 10);  

    thread_handles = malloc (thread_count*sizeof(pthread_t)); 

    thread_data* tdata;
    tdata = malloc (thread_count*sizeof(thread_data)); 


    double time = omp_get_wtime();

    pthread_barrier_init (&barrier, NULL, thread_count);


    for (thread = 0; thread < thread_count; thread++)  {
        tdata[thread].rank = thread;
        tdata[thread].width = width;
        tdata[thread].height = height;
        tdata[thread].g = g;
        tdata[thread].img = img;
        pthread_create(&thread_handles[thread], NULL,
          dt, (void*) &(tdata[thread]));  
    }

    for (thread = 0; thread < thread_count; thread++) 
      pthread_join(thread_handles[thread], NULL); 


    time = omp_get_wtime() - time;
    printf("%f\n", time);


    if(DTGET_QUERY_MAXENTRY_ENABLED())
       DTGET_QUERY_MAXENTRY(g,width*height);

    for (thread = 0; thread < thread_count; thread++)  {
        pthread_create(&thread_handles[thread], NULL,
          encontraMax, (void*) &(tdata[thread]));  
    }


    unsigned int max = 0;
    for (thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
        if(tdata[thread].max > max) max = tdata[thread].max ;
    }
        if(DTGET_QUERY_MAXRETURN_ENABLED())
       DTGET_QUERY_MAXRETURN(max);
printf("%d\n", max);
    divisor = 255.0/max;

    
    if(DTGET_QUERY_TRANSFORMENTRY_ENABLED())
       DTGET_QUERY_TRANSFORMENTRY();

    for (thread = 0; thread < thread_count; thread++)  
        pthread_create(&thread_handles[thread], NULL,
          transformaGS, (void*) &(tdata[thread]));  


    for (thread = 0; thread < thread_count; thread++) 
        pthread_join(thread_handles[thread], NULL); 


    if(DTGET_QUERY_TRANSFORMRETURN_ENABLED())
       DTGET_QUERY_TRANSFORMRETURN();

    free(thread_handles);
    free(tdata);

    stbi_write_png("output.png", width, height, 1, img, width);
}
