#ifndef LCS_H
#define LCS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>

typedef unsigned short mtype;

/* Representa um bloco da matriz LCS */
struct Bloco  {
    int inicio_i;
    int inicio_j;
    int altura;
    int largura;

    mtype *front_cima;
    mtype *front_baixo;
    mtype *front_esq;
    mtype *front_dir;
};

/* Representa uma sequência de caracteres */
struct Sequencia {
    char *dados;
    int tam;
};

/* Calcula o LCS utilizando wavefront por blocos */
mtype lcs_wavefront(const struct Sequencia *A, const struct Sequencia *B, int tam_bloco);

#endif

