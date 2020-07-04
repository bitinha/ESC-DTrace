#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#define BS 4

void dt(unsigned char** imgs, unsigned int** gs, int width, int height, int rank, int n_proc){

    unsigned int MAX = width + height;  
    MPI_Request request;
    MPI_Status status;

    int colunas,linhas;
    int linhas_bloco = height/n_proc;
    int colunas_bloco = width/n_proc;
    int resto_linhas = height % n_proc;
    int resto_colunas = width % n_proc;
    int vizinho_inferior = (n_proc + rank - 1) % n_proc; //Adiciona-se n_proc para evitar valor negativo
    int vizinho_superior = (rank + 1) % n_proc;
    unsigned char *img = imgs[0];
    unsigned int *g = gs[0];

    linhas = linhas_bloco;
    colunas = (rank == n_proc-1) ? colunas_bloco + resto_colunas : colunas_bloco;

    int x;

    for (int y = 0; y < colunas; y++) {
        if (img[y] == 0){
            g[y] = 0;
        } 
        else{
            g[y] = MAX;
        }

        for (x = 1; x < BS; x++) {
            if (img[x * colunas + y] == 0) {
                g[x * colunas + y] = 0;
            }else{
                g[x * colunas + y] = g[(x-1) * colunas + y] + 1;       
            }
        }
    }  

    for(x = BS; x <= linhas - BS; x += BS){
        for (int y = 0; y < colunas; y++) {

            for (int xx = x; xx < x + BS; xx++) {
                if (img[xx * colunas + y] == 0) {
                    g[xx * colunas + y] = 0;
                }
                else {
                    g[xx * colunas + y] = 1 + g[(xx-1) * colunas + y];
                }
            }
        }
    }

    //terminar o ultimo bloco
    for (int y = 0; y < colunas; y++) {

        for (int xx = x; xx < linhas; xx++) {
            if (img[xx * colunas + y] == 0) {
                g[xx * colunas + y] = 0;
            }
            else {
                g[xx * colunas + y] = 1 + g[(xx-1) * colunas + y];
            }
        }
    }
    // for (int y = 0; y < colunas; y++)
    // {
    //     if(img[y] == 0){
    //         g[y] = 0;
    //     }else{
    //         g[y] = MAX;
    //     }

    //     for (int x = 1; x < linhas; x++)
    //     {
    //         if (img[x * colunas + y] == 0) {
    //             g[x * colunas + y] = 0;
    //         }else{
    //             g[x * colunas + y] = g[(x-1) * colunas + y] + 1;       
    //         }

    //     }
    // }
    MPI_Isend(&(g[(linhas-1)*colunas]),colunas,MPI_UNSIGNED,vizinho_inferior,0,MPI_COMM_WORLD,&request);

    unsigned int *recv_buf = malloc((colunas_bloco+resto_colunas)*sizeof(unsigned int));

    for (int f = 1; f < n_proc-1; f++) //Itera pelos diferentes fragmentos da matriz
    {
        colunas = (rank == n_proc-1 - f) ? colunas_bloco + resto_colunas : colunas_bloco;
        img = imgs[f];
        g = gs[f];

        MPI_Recv(recv_buf,colunas,MPI_UNSIGNED,vizinho_superior,f-1,MPI_COMM_WORLD,&status);

        for (int y = 0; y < colunas; y++) {
            if (img[y] == 0){
                g[y] = 0;
            } 
            else{
                g[y] = recv_buf[y]+1;
            }

            for (x = 1; x < BS; x++) {
                if (img[x * colunas + y] == 0) {
                    g[x * colunas + y] = 0;
                }else{
                    g[x * colunas + y] = g[(x-1) * colunas + y] + 1;       
                }
            }
        }  

        for(x = BS; x <= linhas - BS; x += BS){
            for (int y = 0; y < colunas; y++) {

                for (int xx = x; xx < x + BS; xx++) {
                    if (img[xx * colunas + y] == 0) {
                        g[xx * colunas + y] = 0;
                    }
                    else {
                        g[xx * colunas + y] = 1 + g[(xx-1) * colunas + y];
                    }
                }
            }
        }

        //terminar o ultimo bloco
        for (int y = 0; y < colunas; y++) {

            for (int xx = x; xx < linhas; xx++) {
                if (img[xx * colunas + y] == 0) {
                    g[xx * colunas + y] = 0;
                }
                else {
                    g[xx * colunas + y] = 1 + g[(xx-1) * colunas + y];
                }
            }
        }

        // for (int y = 0; y < colunas; y++)
        // {
        //     if(img[y] == 0){
        //         g[y] = 0;
        //     }else{
        //         g[y] = recv_buf[y]+1;
        //     }

        //     for (int x = 1; x < linhas; x++)
        //     {
        //         if (img[x * colunas + y] == 0) {
        //             g[x * colunas + y] = 0;
        //         }else{
        //             g[x * colunas + y] = g[(x-1) * colunas + y] + 1;       
        //         }

        //     }
        // }

        MPI_Isend(&(g[(linhas-1)*colunas]),colunas,MPI_UNSIGNED,vizinho_inferior,f,MPI_COMM_WORLD,&request);

    }

    linhas += resto_linhas;
    colunas = (rank == 0) ? colunas_bloco + resto_colunas : colunas_bloco;
    img = imgs[n_proc-1];
    g = gs[n_proc-1];

    MPI_Recv(recv_buf,colunas,MPI_UNSIGNED,vizinho_superior,n_proc-2,MPI_COMM_WORLD,&status);

    for (int y = 0; y < colunas; y++) {
        if (img[y] == 0){
            g[y] = 0;
        } 
        else{
            g[y] = recv_buf[y]+1;
        }

        for (x = 1; x < BS; x++) {
            if (img[x * colunas + y] == 0) {
                g[x * colunas + y] = 0;
            }else{
                g[x * colunas + y] = g[(x-1) * colunas + y] + 1;       
            }
        }
    }  

    for(x = BS; x <= linhas - BS; x += BS){
        for (int y = 0; y < colunas; y++) {

            for (int xx = x; xx < x + BS; xx++) {
                if (img[xx * colunas + y] == 0) {
                    g[xx * colunas + y] = 0;
                }
                else {
                    g[xx * colunas + y] = 1 + g[(xx-1) * colunas + y];
                }
            }
        }
    }

    //terminar o ultimo bloco
    for (int y = 0; y < colunas; y++) {

        for (int xx = x; xx < linhas; xx++) {
            if (img[xx * colunas + y] == 0) {
                g[xx * colunas + y] = 0;
            }
            else {
                g[xx * colunas + y] = 1 + g[(xx-1) * colunas + y];
            }
        }
    }
    // for (int y = 0; y < colunas; y++)
    // {
    //     if(img[y] == 0){
    //         g[y] = 0;
    //     }else{
    //         g[y] = recv_buf[y]+1;
    //     }

    //     for (int x = 1; x < linhas; x++)
    //     {
    //         if (img[x * (colunas ) + y] == 0) {
    //             g[x * (colunas ) + y] = 0;
    //         }else{
    //             g[x * (colunas ) + y] = g[(x-1) * (colunas ) + y] + 1;       
    //         }

    //     }
    // }

    // 2ª fase do algoritmo

    for (int y = 0; y < colunas; y++) {

        for (int x = linhas - 2; x >= linhas-BS; x--) {
            if (g[(x+1) * colunas + y] < g[x * colunas + y]) {
                g[x * colunas + y] = 1 + g[(x+1) * colunas + y];
            }
        }
    }

    for (x = linhas - BS; x >= BS; x -= BS){

        for (int y = 0; y < colunas; y++) {

            for (int xx = x - 1; xx >= x-BS; xx--) {
                if (g[(xx+1) * colunas + y] < g[xx * colunas + y]) {
                    g[xx * colunas + y] = 1 + g[(xx+1) * colunas + y];
                }
            }
        }
    }

    for (int y = 0; y < colunas; y++) {

        for (int xx = x - 1; xx >= 0; xx--) {
            if (g[(xx+1) * colunas + y] < g[xx * colunas + y]) {
                g[xx * colunas + y] = 1 + g[(xx+1) * colunas + y];
            }
        }
    }
    // for (int y = 0; y < colunas; y++)
    // {
    //     for (int x = linhas-2; x >= 0; x--)
    //     {
    //         if (g[x * colunas + y] > g[(x+1) * colunas + y] + 1) {
    //             g[x * colunas + y] = g[(x+1) * colunas + y] + 1;
    //         }
    //     }
    // }

    linhas = linhas_bloco;

    for (int f = n_proc-2; f >= 0; f--) //Itera pelos diferentes fragmentos da matriz
    {
        MPI_Isend(g,colunas,MPI_UNSIGNED,vizinho_superior,f+1,MPI_COMM_WORLD,&request);
        colunas = (rank == n_proc-1 - f) ? colunas_bloco + resto_colunas : colunas_bloco;
        img = imgs[f];
        g = gs[f];

        MPI_Recv(recv_buf,colunas,MPI_UNSIGNED,vizinho_inferior,f+1,MPI_COMM_WORLD,&status);

        for (int y = 0; y < colunas; y++) {

            if(g[(linhas-1)*colunas+y] > recv_buf[y]+1){
                g[(linhas-1)*colunas+y] = recv_buf[y] + 1;
            }
            for (int x = linhas - 2; x >= linhas-BS; x--) {
                if (g[(x+1) * colunas + y] < g[x * colunas + y]) {
                    g[x * colunas + y] = 1 + g[(x+1) * colunas + y];
                }
            }
        }

        for (x = linhas - BS; x >= BS; x -= BS){

            for (int y = 0; y < colunas; y++) {

                for (int xx = x - 1; xx >= x-BS; xx--) {
                    if (g[(xx+1) * colunas + y] < g[xx * colunas + y]) {
                        g[xx * colunas + y] = 1 + g[(xx+1) * colunas + y];
                    }
                }
            }
        }

        for (int y = 0; y < colunas; y++) {

            for (int xx = x - 1; xx >= 0; xx--) {
                if (g[(xx+1) * colunas + y] < g[xx * colunas + y]) {
                    g[xx * colunas + y] = 1 + g[(xx+1) * colunas + y];
                }
            }
        }
        // for (int y = 0; y < colunas; y++)
        // {
        //     if(g[(linhas-1)*colunas+y] > recv_buf[y]+1){
        //         g[(linhas-1)*colunas+y] = recv_buf[y] + 1;
        //     }

        //     for (int x = linhas-2; x >= 0; x--)
        //     {
        //         if (g[x * colunas + y] > g[(x+1) * colunas + y]+1) {
        //             g[x * colunas + y] = g[(x+1) * colunas + y]+1;
        //         }
        //     }
        // }

    }

    free(recv_buf);
    // 3º Passo
    recv_buf = malloc((linhas_bloco+resto_linhas)*sizeof(unsigned int));

    MPI_Datatype c_blocktype;           //Para colunas por bloco
    MPI_Datatype c_blocktype2;
    MPI_Datatype crl_blocktype;           //Para colunas por bloco
    MPI_Datatype crl_blocktype2;
    MPI_Datatype cr_blocktype;          //Para colunas por bloco mais resto
    MPI_Datatype cr_blocktype2;
    MPI_Datatype crrl_blocktype;          //Para colunas por bloco mais resto
    MPI_Datatype crrl_blocktype2;

    MPI_Type_vector(linhas_bloco, 1, colunas_bloco, MPI_UNSIGNED, &c_blocktype2); 
    MPI_Type_create_resized( c_blocktype2, 0, sizeof(unsigned int), &c_blocktype);
    MPI_Type_commit(&c_blocktype);

    MPI_Type_vector(linhas_bloco+resto_linhas, 1, colunas_bloco, MPI_UNSIGNED, &crl_blocktype2); 
    MPI_Type_create_resized( crl_blocktype2, 0, sizeof(unsigned int), &crl_blocktype);
    MPI_Type_commit(&crl_blocktype);

    MPI_Type_vector(linhas_bloco, 1, colunas_bloco+resto_colunas, MPI_UNSIGNED, &cr_blocktype2); 
    MPI_Type_create_resized( cr_blocktype2, 0, sizeof(unsigned int), &cr_blocktype);
    MPI_Type_commit(&cr_blocktype);

    MPI_Type_vector(linhas_bloco+resto_linhas, 1, colunas_bloco+resto_colunas, MPI_UNSIGNED, &crrl_blocktype2); 
    MPI_Type_create_resized( crrl_blocktype2, 0, sizeof(unsigned int), &crrl_blocktype);
    MPI_Type_commit(&crrl_blocktype);
    
    MPI_Datatype datatype;

    g=gs[(n_proc-rank)%n_proc];

    colunas = colunas_bloco;
    if (rank == 1) {
        datatype = crl_blocktype;
        linhas = linhas_bloco + resto_linhas;
    }else{
        datatype = c_blocktype;
        linhas = linhas_bloco;
    }

    for (int x = 0; x < linhas; x++)
    {
        for (int y = 1; y < colunas; y++)
        {
            if (g[x * colunas + y] > g[x * colunas + (y-1)] + 1) {
                g[x * colunas + y] = g[x * colunas + (y-1)] + 1;
            }
        }
    }


    for (int f = 1; f < n_proc-1; f++) //Itera pelos diferentes fragmentos da matriz
    {
        MPI_Isend(&(g[(colunas-1)]),1,datatype,vizinho_superior,f-1,MPI_COMM_WORLD,&request);

        if (rank == f+1) {
            datatype = crl_blocktype;
            linhas = linhas_bloco + resto_linhas;
        }else{
            datatype = c_blocktype;
            linhas = linhas_bloco;
        }
        g = gs[(n_proc-rank +f)%n_proc];

        MPI_Recv(recv_buf,linhas,MPI_UNSIGNED,vizinho_inferior,f-1,MPI_COMM_WORLD,&status);
        for (int x = 0; x < linhas; x++)
        {
            if(g[x*colunas] > recv_buf[x]+1){
                g[x*colunas] = recv_buf[x] + 1;
            }

            for (int y = 1; y < colunas; y++)
            {
                if (g[x * colunas + y] > g[x * colunas + y-1]+1) {
                    g[x * colunas + y] = g[x * colunas + y-1]+1;
                }
            }
        }
    }



    MPI_Isend(&(g[(colunas_bloco-1)]),1,datatype,vizinho_superior,n_proc-2,MPI_COMM_WORLD,&request);

    colunas += resto_colunas;

    if (rank == 0) {
        datatype = crrl_blocktype;
        linhas = linhas_bloco + resto_linhas;
    }else{
        datatype = cr_blocktype;
        linhas = linhas_bloco;
    }
    g = gs[n_proc-1-rank];

    MPI_Recv(recv_buf, linhas, MPI_UNSIGNED, vizinho_inferior, n_proc-2, MPI_COMM_WORLD,&status);


    for (int x = 0; x < linhas; x++)
    {
        if(g[x*colunas] > recv_buf[x]+1){
            g[x*colunas] = recv_buf[x] + 1;
        }

        for (int y = 1; y < colunas; y++)
        {
            if (g[x * colunas + y] > g[x * colunas + y-1]+1) {
                g[x * colunas + y] = g[x * colunas + y-1]+1;
            }
        }
    }

    // 4º Passo

    for (int x = 0; x < linhas; x++)
    {
        for (int y = colunas-2; y >= 0; y--)
        {
            if (g[x * colunas + y] > g[x * colunas + y+1]+1) {
                g[x * colunas + y] = g[x * colunas + y+1]+1;
            }
        }
    }

    MPI_Isend(g,1,datatype,vizinho_inferior,n_proc-2,MPI_COMM_WORLD,&request);

    colunas = colunas_bloco;

    for (int f = n_proc-2; f > 0; f--) //Itera pelos diferentes fragmentos da matriz
    {

        if (rank == f+1) {
            datatype = crl_blocktype;
            linhas = linhas_bloco + resto_linhas;
        }else{
            datatype = c_blocktype;
            linhas = linhas_bloco;
        }
        g = gs[(n_proc-rank +f)%n_proc];

        MPI_Recv(recv_buf,linhas,MPI_UNSIGNED,vizinho_superior,f,MPI_COMM_WORLD,&status);

        for (int x = 0; x < linhas; x++)
        {
            if(g[x*colunas+colunas-1] > recv_buf[x]+1){
                g[x*colunas+colunas-1] = recv_buf[x] + 1;
            }

            for (int y = colunas-2; y >= 0; y--)
            {
                if (g[x * colunas + y] > g[x * colunas + y+1]+1) {
                    g[x * colunas + y] = g[x * colunas + y+1]+1;
                }
            }
        }
        MPI_Isend(g,1,datatype,vizinho_inferior,f-1,MPI_COMM_WORLD,&request);
    }

    linhas = (rank == 1) ? linhas_bloco + resto_linhas : linhas_bloco;
    g = gs[(n_proc-rank)%n_proc];

    MPI_Recv(recv_buf,linhas,MPI_UNSIGNED,vizinho_superior,0,MPI_COMM_WORLD,&status);

    for (int x = 0; x < linhas; x++)
    {
        if(g[x*colunas+colunas-1] > recv_buf[x]+1){
            g[x*colunas+colunas-1] = recv_buf[x] + 1;
        }

        for (int y = colunas-2; y >= 0; y--)
        {
            if (g[x * colunas + y] > g[x * colunas + y+1]+1) {
                g[x * colunas + y] = g[x * colunas + y+1]+1;
            }
        }
    }

}

unsigned int encontraMax(unsigned int** out, int width, int height, int rank, int n_proc) {

    unsigned int local_max = 0;
    unsigned int global_max = 0;


    int colunas;
    int linhas = height/n_proc;
    for(int f = 0; f < n_proc-1; f++){
        colunas = (n_proc-1-f == rank) ? (width / n_proc) + (width%n_proc) : width / n_proc; 
        for (int i = 0; i < colunas * linhas; i++) {
            if (out[f][i] > local_max) {
                local_max = out[f][i];
            }
        }
    }

    linhas += height%n_proc;
    colunas = (0 == rank) ? (width / n_proc) + (width%n_proc) : width / n_proc; 
    for (int i = 0; i < colunas*linhas; i++){
        if (out[n_proc-1][i] > local_max) {
            local_max = out[n_proc-1][i];
        }
    }

    MPI_Allreduce(&local_max, &global_max, 1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);

    return global_max;
}

void transformaGS(unsigned char** img, unsigned int** out, int width, int height, int rank, int n_proc) {
    unsigned int max = encontraMax(out,width,height,rank,n_proc);
    double div;

    div = 255.0/max;

    int colunas;
    int linhas = height/n_proc;
    for(int f = 0; f < n_proc-1; f++){
        colunas = (n_proc-1-f == rank) ? (width / n_proc) + (width%n_proc) : width / n_proc; 
        for (int i = 0; i < colunas * linhas; i++) {
            img[f][i] = out[f][i] * div;
        }
    }
    linhas += height%n_proc;
    colunas = (0 == rank) ? (width / n_proc) + (width%n_proc) : width / n_proc; 
    for (int i = 0; i < colunas*linhas; i++){
        img[n_proc-1][i] = out[n_proc-1][i]*div;
    }

}


int main(int argc, char* argv[]) {
    double t1, t2;

    int rank, n_proc;
    int inicio, fim;

    MPI_Status status;
    MPI_Init(&argc, &argv); //Initializes the library
    MPI_Comm_size(MPI_COMM_WORLD, &n_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //Get the id of the current process

    int tamanhos[2];
    unsigned int *g;
    unsigned char *img;
    /*unsigned char img[] = {0,0,0,0,0,0,0,0,0,0,0,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,1,0};*/
    int width, height;
    if(rank==0){

        int channels;
        if(argc<2){
            printf("Indique uma imagem\n");
            exit(2);

        }
        img = stbi_load(argv[1], &width, &height, &channels, 0);
        //width=12;
        //height=12;
        if (img == NULL) {
            printf("Error in loading the image\n");
            exit(1);
        }
        printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

        g = malloc(width * height * sizeof(unsigned int));
        tamanhos[0] = width;
        tamanhos[1] = height;
        MPI_Bcast(tamanhos, 2, MPI_INT, 0, MPI_COMM_WORLD);

    }else{

        MPI_Bcast(tamanhos, 2, MPI_INT, 0, MPI_COMM_WORLD);

        width = tamanhos[0];
        height = tamanhos[1];

    }

    // int colunas_por_processo = width / n_proc;
    // if(n_proc-1 == rank){
    //     colunas_por_processo += width % n_proc;    // ultimo processo fica tambem com o resto
    // }

    //int bs = (width * height) / n_proc;
    //int bv = (height/10);

    unsigned int *fs[n_proc];
    unsigned char *imgfs[n_proc];

    int linhas_bloco = height/n_proc;
    int colunas_bloco = width/n_proc;
    int resto_linhas = height % n_proc;
    int resto_colunas = width % n_proc;

    for (int i = 0; i < n_proc-1; i++) {
        if (n_proc-1-i == rank) {
            fs[i] = malloc(((colunas_bloco + resto_colunas) * linhas_bloco) * sizeof(unsigned int));
            imgfs[i] = malloc(((colunas_bloco + resto_colunas) * linhas_bloco) * sizeof(unsigned char));
        }
        else {
            fs[i] = malloc(colunas_bloco * linhas_bloco * sizeof(unsigned int));
            imgfs[i] = malloc((colunas_bloco * linhas_bloco) * sizeof(unsigned char));
        }
    }

    if (rank == 0) {
        fs[n_proc-1] = malloc((colunas_bloco + resto_colunas) * (linhas_bloco + resto_linhas) * sizeof(unsigned int));
        imgfs[n_proc-1] = malloc((colunas_bloco + resto_colunas) * (linhas_bloco + resto_linhas) * sizeof(unsigned char));
    }
    else {
        fs[n_proc-1] = malloc(colunas_bloco * (linhas_bloco + resto_linhas) * sizeof(unsigned int));
        imgfs[n_proc-1] = malloc(colunas_bloco * (linhas_bloco + resto_linhas) * sizeof(unsigned char));
    }


    MPI_Datatype std_blocktype;
    MPI_Datatype std_blocktype2;
    MPI_Datatype rc_blocktype; // Para o resto das colunas
    MPI_Datatype rc_blocktype2;
    MPI_Datatype rl_blocktype; // Para o resto das linhas
    MPI_Datatype rl_blocktype2;

    MPI_Type_vector(linhas_bloco, colunas_bloco, width, MPI_UNSIGNED_CHAR, &std_blocktype2); 
    MPI_Type_create_resized( std_blocktype2, 0, sizeof(unsigned char), &std_blocktype);
    MPI_Type_commit(&std_blocktype);

    MPI_Type_vector(linhas_bloco, colunas_bloco + resto_colunas, width, MPI_UNSIGNED_CHAR, &rc_blocktype2);
    MPI_Type_create_resized( rc_blocktype2, 0, sizeof(unsigned char), &rc_blocktype);
    MPI_Type_commit(&rc_blocktype);

    MPI_Type_vector(linhas_bloco + resto_linhas, colunas_bloco, width, MPI_UNSIGNED_CHAR, &rl_blocktype2);
    MPI_Type_create_resized( rl_blocktype2, 0, sizeof(unsigned char), &rl_blocktype);
    MPI_Type_commit(&rl_blocktype);

    int displs[n_proc];
    int counts[n_proc];
    int elementos_recebidos;
    t1 = MPI_Wtime();
    for (int i = 0; i < n_proc-1; i++) {  // Itera pelas linhas divisoras da matriz
        if(rank == 0){
            for(int p = 0; p<n_proc; p++){
                counts[p] = (n_proc-1-p == i) ? 0 : 1;       //Um dos processos vai receber o resto das colunas mais tarde
                displs[p] = i*width*linhas_bloco+((p+i)%n_proc)*colunas_bloco;
            }
        }

        elementos_recebidos = (n_proc-1-i == rank) ? 0 : linhas_bloco * colunas_bloco; //Quantidade de elementos que serão recebidos nesta iteração


        MPI_Scatterv(img, counts, displs, std_blocktype, imgfs[i], elementos_recebidos, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    }
    //Ultima linha
    if(rank == 0){
        for(int p = 0; p<n_proc; p++){
            counts[p] = (p == 0) ? 0 : 1;       //Um dos processos vai receber o resto das colunas mais tarde
            displs[p] = (n_proc-1)*width*linhas_bloco+((n_proc-1+p)%n_proc)*colunas_bloco;
        }
    }

    elementos_recebidos = (rank==0) ? 0 : (linhas_bloco + resto_linhas) * colunas_bloco; //Quantidade de elementos que serão recebidos nesta iteração


    MPI_Scatterv(img, counts, displs, rl_blocktype, imgfs[n_proc-1], elementos_recebidos, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    //Ultima coluna
    elementos_recebidos = (rank==0) ? 0 : linhas_bloco * (colunas_bloco + resto_colunas); //Quantidade de elementos que serão recebidos nesta iteração

    if(rank == 0){
        for(int p = 0; p<n_proc; p++){
            displs[p] = (n_proc-1-p)*width*linhas_bloco+colunas_bloco*(n_proc-1);
        }
    }

    MPI_Scatterv(img, counts, displs, rc_blocktype, imgfs[n_proc-1-rank], elementos_recebidos, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    if (rank == 0){
        for (int i = 0; i < linhas_bloco + resto_linhas; i++)
        {
            for (int j = 0; j < colunas_bloco + resto_colunas; j++)
            {
                imgfs[n_proc-1][i*(colunas_bloco + resto_colunas)+j] = img[(width*((n_proc-1)*linhas_bloco+i))+(n_proc-1)*colunas_bloco+j];
            }
        }
    }
    t2 = MPI_Wtime();

    if (rank == 0) printf("t scatter: %f\n",t2-t1);

    //Matrizes recebidas estarão transpostas
    t1 = MPI_Wtime();
    dt(imgfs,fs,width,height,rank,n_proc);
    t2 = MPI_Wtime();
    if (rank == 0) printf("t dt: %f\n",t2-t1);

    transformaGS(imgfs, fs, width,height,rank,n_proc);

    t1 = MPI_Wtime();
    for (int i = 0; i < n_proc-1; i++) {  // Itera pelas linhas divisoras da matriz
        if(rank == 0){
            for(int p = 0; p<n_proc; p++){
                counts[p] = (n_proc-1-p == i) ? 0 : 1;       //Um dos processos vai receber o resto das colunas mais tarde
                displs[p] = i*width*linhas_bloco+((p+i)%n_proc)*colunas_bloco;
            }
        }

        elementos_recebidos = (n_proc-1-i == rank) ? 0 : linhas_bloco * colunas_bloco; //Quantidade de elementos que serão recebidos nesta iteração


        MPI_Gatherv(imgfs[i], elementos_recebidos, MPI_UNSIGNED_CHAR, img, counts, displs, std_blocktype, 0, MPI_COMM_WORLD);

    }
    //Ultima linha
    if(rank == 0){
        for(int p = 0; p<n_proc; p++){
            counts[p] = (p == 0) ? 0 : 1;       //Um dos processos vai receber o resto das colunas mais tarde
            displs[p] = (n_proc-1)*width*linhas_bloco+((n_proc-1+p)%n_proc)*colunas_bloco;
        }
    }
    elementos_recebidos = (rank==0) ? 0 : (linhas_bloco+resto_linhas) * colunas_bloco; //Quantidade de elementos que serão recebidos nesta iteração

    
    MPI_Gatherv(imgfs[n_proc-1], elementos_recebidos, MPI_UNSIGNED_CHAR, img, counts, displs, rl_blocktype, 0, MPI_COMM_WORLD);

    //Ultima coluna

    if(rank == 0){
        for(int p = 0; p<n_proc; p++){
            displs[p] = (n_proc-1-p)*width*linhas_bloco+colunas_bloco*(n_proc-1);
        }
    }
    elementos_recebidos = (rank==0) ? 0 : linhas_bloco * (colunas_bloco+resto_colunas); //Quantidade de elementos que serão recebidos nesta iteração

    MPI_Gatherv(imgfs[n_proc-1-rank], elementos_recebidos, MPI_UNSIGNED_CHAR, img, counts, displs, rc_blocktype, 0, MPI_COMM_WORLD);

    if(rank == 0){
        for (int i = 0; i < linhas_bloco + resto_linhas; i++)
        {
            for (int j = 0; j < colunas_bloco + resto_colunas; j++)
            {
                img[(width*((n_proc-1)*linhas_bloco+i))+(n_proc-1)*colunas_bloco+j] = imgfs[n_proc-1][i*(colunas_bloco + resto_colunas)+j];
            }
        }
    }


    t2 = MPI_Wtime();
    if (rank == 0) printf("t gather: %f\n",t2-t1);

    if(rank == 0){
        stbi_write_jpg("output3.jpg", width, height, 1, img, 100);
        // stbi_write_png("output3.png", width, height, 1, img, width);
    }

    MPI_Finalize();


    return 0;
}
