#include <stdio.h>
#include "lcs.h"


int calcular_diagonal(
    struct Bloco *blocos,
    int num_blocos_linhas,
    int num_blocos_colunas,
    int diag,
    int rank,
    int num_processos,
    const struct Sequencia *A,
    const struct Sequencia *B,
    mtype **buffers) {

    int erro_local = 0;

    int inicio = diag - (num_blocos_colunas - 1);

    if (inicio < 0)
        inicio = 0;

    int fim = diag;

    if (fim >= num_blocos_linhas)
        fim = num_blocos_linhas - 1;


    #pragma omp parallel for schedule(static) reduction(|:erro_local)
    for (int i = inicio; i <= fim; i++) {

        int j = diag - i;

        int indice = i * num_blocos_colunas + j;

        struct Bloco *cima = NULL;
        struct Bloco *esquerda = NULL;

        if (i > 0) {
            cima = &blocos[(i - 1) * num_blocos_colunas + j];
        }

        if (j > 0) {
            esquerda = &blocos[i * num_blocos_colunas + (j - 1)];
        }

        int dono = dono_bloco(i, num_blocos_linhas, num_processos);

        if (dono == rank) {
            int tid = omp_get_thread_num();

            mtype *matriz = buffers[tid];

            if (calcular_bloco(&blocos[indice], A, B, cima, esquerda, matriz) != 0) {
                fprintf(stderr, "Erro ao calcular bloco (%d, %d)\n", i, j);
                erro_local = 1;
            }
        }
    }

    return erro_local ? -1 : 0;
}