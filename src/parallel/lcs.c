#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcs.h"


// Libera a memória utilizada pelos blocos
void libera_blocos(struct Bloco *blocos, int qtde) {

    if (blocos == NULL)
        return;

    for (int k = 0; k < qtde; k++) {
        free(blocos[k].front_baixo);
        free(blocos[k].front_dir);
    }

    free(blocos);
}


// Calcula um único bloco da matriz LCS
int calcular_bloco(
    struct Bloco *bloco,
    const struct Sequencia *A,
    const struct Sequencia *B,
    const struct Bloco *bloco_cima,
    const struct Bloco *bloco_esq,
    mtype *matriz) {

    int h = bloco->altura;
    int l = bloco->largura;

    int colunas = l + 1;

    // Limpa a área da matriz que será utilizada neste bloco
    memset(matriz, 0, (size_t)(h + 1) * (l + 1) * sizeof(mtype));

    // Inicializa a primeira linha com a fronteira do bloco de cima
    if (bloco_cima != NULL) {

        for (int j = 0; j <= l; j++) {
            matriz[j] = bloco_cima->front_baixo[j];
        }
    }

    // Inicializa a primeira coluna com a fronteira do bloco à esquerda
    if (bloco_esq != NULL) {

        for (int i = 1; i <= h; i++) {
            matriz[i * colunas] = bloco_esq->front_dir[i];
        }
    }

    // Calcula as células internas do bloco
    for (int i = 1; i <= h; i++) {

        for (int j = 1; j <= l; j++) {

            int indice = i * colunas + j;

            int pos_A = bloco->inicio_j + j - 2;
            int pos_B = bloco->inicio_i + i - 2;

            if (A->dados[pos_A] == B->dados[pos_B]) {

                matriz[indice] = matriz[(i - 1) * colunas + (j - 1)] + 1;

            } else {

                mtype cima = matriz[(i - 1) * colunas + j];
                mtype esquerda = matriz[i * colunas + (j - 1)];

                matriz[indice] = (cima > esquerda) ? cima : esquerda;
            }
        }
    }

    // Armazena a fronteira inferior
    for (int j = 0; j <= l; j++) {
        bloco->front_baixo[j] = matriz[h * colunas + j];
    }

    // Armazena a fronteira direita
    for (int i = 0; i <= h; i++) {
        bloco->front_dir[i] = matriz[i * colunas + l];
    }

    return 0;
}