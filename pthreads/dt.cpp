#include <iostream>
#include <new>
#include <chrono>
#include <pthread.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

//Otimização para aproveitar a localidade espacial ao percorrer as linhas
#define bs 4  //L1d SIZE = 32KiB; int = 4B; ints que cabem na cache = 8192; Assumindo o tamanho de linha da cache como 64B, 16 ints por linha
// nº de linhas = 512
int ls = 0;

int width, height, channels;
unsigned int *g;
unsigned char *img;
int thread_count;

int encontraMax(unsigned int *__restrict__ out, int N) {
    unsigned int max = 0;

    for (int i = 0; i < N; i++) {
        if (out[i] > max) {
            max = out[i];
        }
    }

    return max;
}

void *dt_para(void *rank) {
    long my_rank = (long) rank;
    int local_height = height / thread_count;
    int my_first_row = my_rank * local_height;
    int my_last_row = (my_rank + 1) * local_height - 1;

    /*
    for (int i = my_first_row; i <= my_last_row; i++) {
        for (int j = 0; j <= width; j++) {
            g[i * width + j] = 3;
        }
    }
    */

    return NULL;
}

void dt(unsigned char *img, unsigned int *g, int width, int height) {
    int max = width + height;

    for (int y = 0; y < width; y++) {
        if (img[y] == 0) {
            g[y] = 0;
        } else {
            g[y] = max;
        }

        for (int x = 1; x < height; x++) {
            if (img[x * width + y] == 0) {
                g[x * width + y] = 0;
            } else {
                g[x * width + y] = 1 + g[(x - 1) * width + y];
            }
        }

        for (int x = height - 2; x >= 0; x--) {
            if (g[(x + 1) * width + y] < g[x * width + y]) {
                g[x * width + y] = 1 + g[(x + 1) * width + y];
            }
        }
    }

    for (int x = 0; x < height; x++) {
        for (int y = 1; y < width; y++) {
            if (g[x * width + y] > g[x * width + (y - 1)] + 1) {
                g[x * width + y] = g[x * width + (y - 1)] + 1;
            }
        }
        for (int y = width - 2; y >= 0 ; y--) {
            if (g[x * width + y] > g[x * width + (y + 1)] + 1) {
                g[x * width + y] = g[x * width + (y + 1)] + 1;
            }
        }
    }
}

void transformaGS(unsigned char *__restrict__ img, unsigned int *__restrict__ out, int N) {
    unsigned int max = encontraMax(out, N);
    std::cout << "Max: " << max << std::endl;
    double div = 255.0 / max;
    for (int i = 0; i < N; i++) {
        img[i] = out[i] * div;
    }
}

int main(int argc, char const *argv[]) {
    if (argc < 3) {
        std::cout << "Indique uma imagem e o número de threads" << std::endl;
        exit(2);
    }

    img = stbi_load(argv[1], &width, &height, &channels, 0);

    if (img == NULL) {
        std::cout << "Error in loading the image" << std::endl;
        exit(1);
    }

    std::cout << "Loaded image with a width of " << width << "px, a height of "
              << height << "px and " << channels << " channels" << std::endl;

    g = new unsigned int[width * height]();

    long thread;
    pthread_t *thread_handles;
    thread_count = strtol(argv[2], NULL, 10);
    thread_handles = new pthread_t[thread_count]();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    dt(img, g, width, height);
    /*
    for (thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL,
                       dt_para, (void *) thread);

    for (thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);
    */
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << " µs" << std::endl;

    transformaGS(img, g,  width * height);
    stbi_write_png("output.png", width, height, 1, img, width);

    delete[] img;
    delete[] g;
    delete[] thread_handles;

    return 0;
}
