#include <iostream>
#include <new>
#include <chrono>
#include <thread>
#include <mutex>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "ddt.h"

//Otimização para aproveitar a localidade espacial ao percorrer as linhas
#define bs 4  //L1d SIZE = 32KiB; int = 4B; ints que cabem na cache = 8192; Assumindo o tamanho de linha da cache como 64B, 16 ints por linha
// nº de linhas = 512

std::mutex m;

void encontraMax(unsigned int *__restrict__ out, int N, unsigned int *max, int my_rank, int thread_count) {
    std::unique_lock<std::mutex> guard(m, std::defer_lock);
    unsigned int local_max = 0;

    int i = (N / thread_count) * my_rank;

    int f = (my_rank != thread_count-1) ? i + N / thread_count : i + N / thread_count + N % thread_count;

    for (; i < f; i++) {
        if (out[i] > local_max) {
            local_max = out[i];
        }
    }
    
    guard.lock();
    if (local_max > *max)
        *max = local_max;
    guard.unlock();
}

void dt_p1(unsigned char *img, unsigned int *g, int width, int height, int my_rank, int thread_count) {
    int max = width + height;
    int x;

    int i_width = (width / thread_count) * my_rank;

    int f_width = (my_rank != thread_count - 1) ? i_width + width / thread_count : i_width + width / thread_count +
                                                                                   width % thread_count;


    //Passo 1

    if(DTGET_QUERY_PRIMEIROPASSO_ENABLED())
       DTGET_QUERY_PRIMEIROPASSO();

    // Preencher distancias no 1º bloco
    for (int y = i_width; y < f_width; y++) {
        if (img[y] == 0) {
            g[y] = 0;
        } else {
            g[y] = max;
        }

        for (int x = 1; x < bs; x++) {
            if (img[x * width + y] == 0) {
                g[x * width + y] = 0;
            } else {
                g[x * width + y] = 1 + g[(x - 1) * width + y];
            }
        }
    }

    for (x = bs; x <= height - bs; x += bs) {
        for (int y = i_width; y < f_width; y++) {

            for (int xx = x; xx < x + bs; xx++) {
                if (img[xx * width + y] == 0) {
                    g[xx * width + y] = 0;
                } else {
                    g[xx * width + y] = 1 + g[(xx - 1) * width + y];
                }
            }
        }
    }

    //terminar o ultimo bloco
    for (int y = i_width; y < f_width; y++) {

        for (int xx = x; xx < height; xx++) {
            if (img[xx * width + y] == 0) {
                g[xx * width + y] = 0;
            } else {
                g[xx * width + y] = 1 + g[(xx - 1) * width + y];
            }
        }
    }

    //Passo 2

    if (DTGET_QUERY_SEGUNDOPASSO_ENABLED())
        DTGET_QUERY_SEGUNDOPASSO();
    // Preencher distancias no 1º bloco (bloco de baixo)
    for (int y = i_width; y < f_width; y++) {

        for (int x = height - 2; x >= height - bs; x--) {
            if (g[(x + 1) * width + y] < g[x * width + y]) {
                g[x * width + y] = 1 + g[(x + 1) * width + y];
            }
        }
    }

    for (x = height - bs; x >= bs; x -= bs) {

        for (int y = i_width; y < f_width; y++) {

            for (int xx = x - 1; xx >= x - bs; xx--) {
                if (g[(xx + 1) * width + y] < g[xx * width + y]) {
                    g[xx * width + y] = 1 + g[(xx + 1) * width + y];
                }
            }
        }
    }

    for (int y = i_width; y < f_width; y++) {

        for (int xx = x - 1; xx >= 0; xx--) {
            if (g[(xx + 1) * width + y] < g[xx * width + y]) {
                g[xx * width + y] = 1 + g[(xx + 1) * width + y];
            }
        }
    }
}
void dt_p2(unsigned int *g, int width, int height, int my_rank, int thread_count) {
    int i_height = (height / thread_count) * my_rank;

    int f_height = (my_rank != thread_count - 1) ? i_height + height / thread_count : i_height + height / thread_count +
                                                                                      height % thread_count;
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
}

void transformaGS(unsigned char *__restrict__ img, unsigned int *__restrict__ out, int N, double divisor, int my_rank, int thread_count) {
    int i = (N / thread_count) * my_rank;

    int f = (my_rank != thread_count-1) ? i + N / thread_count : i + N / thread_count + N % thread_count;

    for (; i < f; i++) {
        img[i] = out[i] * divisor;
    }
}

int main(int argc, char const *argv[]) {
    int width, height, channels;
    //unsigned int THREADS_NUMBER = std::thread::hardware_concurrency();
    unsigned int THREADS_NUMBER = strtol(argv[2], NULL, 10);  

    std::thread threads[THREADS_NUMBER];

    if (argc < 2) {
        std::cout << "Indique uma imagem" << std::endl;
        exit(2);
    }

    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);

    if (img == nullptr) {
        std::cout << "Error in loading the image" << std::endl;
        exit(1);
    }

    std::cout << "Loaded image with a width of " << width << "px, a height of "
              << height << "px and " << channels << " channels" << std::endl;

    auto *g = new unsigned int[width * height]();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        threads[i] = std::thread(dt_p1, img, g, width, height, i, THREADS_NUMBER);
    }

    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        // Wait until each thead has finished
        threads[i].join();
    }

    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        threads[i] = std::thread(dt_p2, g, width, height, i, THREADS_NUMBER);
    }

    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        // Wait until each thead has finished
        threads[i].join();
    }

    if(DTGET_QUERY_MAXENTRY_ENABLED())
       DTGET_QUERY_MAXENTRY(g,width*height);
    unsigned int max = 0;
    
    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        threads[i] = std::thread(encontraMax, g, width * height, &max, i, THREADS_NUMBER);
    }

    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        // Wait until each thead has finished
        threads[i].join();
    }

    
    if(DTGET_QUERY_MAXRETURN_ENABLED())
       DTGET_QUERY_MAXRETURN(max);
    std::cout << "Max: " << max << std::endl;
    double divisor = 255.0/max;


    if(DTGET_QUERY_TRANSFORMENTRY_ENABLED())
       DTGET_QUERY_TRANSFORMENTRY();

    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        threads[i] = std::thread(transformaGS, img, g, width * height, divisor, i, THREADS_NUMBER);
    }

    for (unsigned int i = 0; i < THREADS_NUMBER; ++i) {
        // Wait until each thead has finished
        threads[i].join();
    }

    if(DTGET_QUERY_TRANSFORMRETURN_ENABLED())
       DTGET_QUERY_TRANSFORMRETURN();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << " µs" << std::endl;

    stbi_write_png("output.png", width, height, 1, img, width);

    delete[] img;
    delete[] g;

    return 0;
}
