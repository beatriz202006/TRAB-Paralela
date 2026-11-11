#include <stdio.h>
#include "lcs.h"

// Libera as fronteiras armazenadas nos blocos
static void libera_blocos(struct Bloco *blocos, int qtde) {
    if (!blocos)
        return;
    
    for (int k = 0; k < qtde; k++) {
        free(blocos[k].front_baixo);
        free(blocos[k].front_cima);
    }

    free(blocos);
}

// Calcula um único bloco da matriz
static int calcular_bloco(struct Bloco *bloco, const struct Sequencia *A, const struct Sequencia *B, const struct Bloco *bloco_cima, const struct Bloco *bloco_esq) {
    int h = bloco->altura;
    int l = bloco->largura;

    // Matriz temporária do bloco
    int colunas = l + 1;

    // Aloca a matriz do bloco inicializando com zeros
    mtype *matriz = calloc((size_t)(h + 1) * (l + 1), sizeof(mtype));

    if (!matriz)
       return -1;
    
    // Inicializa a primeira linha com a fronteira superior
    if (bloco_cima != NULL) {
        for (int j = 0; j <= l; j++) {
            matriz[j] = bloco_cima->front_baixo[j]
        }
    }

    // Inicializa a primeira coluna com a fronteira esquerda
    if (bloco_esq != NULL) {
        for (int i = 1; i <= h; i++) {
            matriz[i * colunas] = bloco_esq->front_dir[i];
        }
    }

    // Calcula as células internas do bloco
    for (int i = 1; i <= h; i++) {
        for (int j = 1; j <= l; j++) {

            // Converte a posição (i,j) da matriz local para um índice do vetor que armazena a matriz
            int indice = i * colunas + j;

            int pos_A = bloco->inicio_j + j - 1;  // posição da seq A que corresponde à coluna atual do bloco
            int pos_B = bloco->inicio_i + i - 1;  // posição da seq B que corresponde à linha atual do bloco

            // Compara os caracteres das sequências A e B
            if (A->dados[pos_A] == B->dados[pos_B]) {
                // Se os caracteres são iguais, incrementa o valor da célula diagonal
                matriz[indice] = matriz[(i-1) * colunas + (j-1)] + 1;
            } else {
                // Se os caracteres são diferentes, pega o maior valor entre a célula de cima e a da esquerda
                mtype cima = matriz[(i-1) * colunas + j];
                mtype esquerda = matriz[i * colunas + (j-1)];
                
                if (cima > esquerda) 
                    matriz[indice] = cima;
                else
                    matriz[indice] = esquerda;
            }
        }
    }

    // Armazena a última linha do bloco
    for (int j = 0; j <= l; j++) {
        bloco->front_baixo[j] = matriz[h * colunas + j];
    }

    // Armazena a última coluna do bloco
    for (int i = 0; i <= h; i++) {
        bloco->front_dir = matriz[i * colunas + w];
    }

    free(matriz);
    return 0; 
}

// Implementação sequencial do wavefront por blocos
mtype lcs_wavefront(const struct Sequencia *A, const struct Sequencia *B, int tam_bloco) {
    if (!A || !B || A->dados || B->dados || tam_bloco <= 0) {
        fprintf(stderr, "Parâmetros inválidos\n");
        return -1;
    }

    int tamA = A->tam;
    int tamB = B->tam;

    if (!tamA || !tamB)
        return -1;

    // Quantidade de blocos em cada dimensão
    int num_blocos_colunas = (tamA + tam_bloco - 1) / tam_bloco;
    int num_blocos_linhas = (tamB + tam_bloco - 1) / tam_bloco;
    int qtde = num_blocos_linhas * num_blocos_colunas;

    // Aloca memória para os blocos e inicializa com zero
    struct Bloco *blocos = calloc(qtde, sizeof(struct Bloco));

    if (!blocos) {
        fprintf(stderr, "Erro ao alocar os blocos\n");
        return -1;
    }

}