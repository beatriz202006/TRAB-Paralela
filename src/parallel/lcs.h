#ifndef LCS_H
#define LCS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>

typedef unsigned short mtype;

/* Representa um bloco da matriz LCS */
struct Bloco {
    int inicio_i;
    int inicio_j;
    int altura;
    int largura;

    mtype *front_baixo;
    mtype *front_dir;
};

/* Representa uma sequência de caracteres */
struct Sequencia {
    char *dados;
    int tam;
};

/* Buffer temporário exclusivo de cada thread OpenMP */
struct BufferThread {
    mtype *dados;
    size_t capacidade;
};

/* Libera a memória utilizada pelos blocos */
void libera_blocos(struct Bloco *blocos, int qtde);

/* Retorna o processo MPI responsável por uma linha de blocos */
int dono_bloco(int linha, int num_blocos_linhas, int num_processos);

/* Calcula um único bloco da matriz LCS */
int calcular_bloco(
    struct Bloco *bloco,
    const struct Sequencia *A,
    const struct Sequencia *B,
    const struct Bloco *bloco_cima,
    const struct Bloco *bloco_esq,
    mtype *matriz);

/* Executa o cálculo dos blocos de uma diagonal utilizando OpenMP */
int calcular_diagonal(
    struct Bloco *blocos,
    int num_blocos_linhas,
    int num_blocos_colunas,
    int diag,
    int rank,
    int num_processos,
    const struct Sequencia *A,
    const struct Sequencia *B,
    mtype **buffers);

/* Realiza a sincronização dos resultados entre os processos MPI */
/* Compartilha as fronteiras dos blocos de uma diagonal entre os processos MPI */
int sincronizar_diagonal(
    struct Bloco *blocos,
    int num_blocos_linhas,
    int num_blocos_colunas,
    int diag,
    int num_processos);

/* Calcula o LCS utilizando wavefront por blocos */
int lcs_wavefront(
    const struct Sequencia *A,
    const struct Sequencia *B,
    int tam_bloco);

#endif

