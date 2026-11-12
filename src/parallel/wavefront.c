#include <stdio.h>
#include <stdlib.h>
#include "lcs.h"


// Libera as fronteiras armazenadas nos blocos
static void libera_blocos(struct Bloco *blocos, int qtde) {

    if (blocos == NULL)
        return;

    // Percorre todos os blocos e libera suas fronteiras
    for (int k = 0; k < qtde; k++) {
        free(blocos[k].front_baixo);
        free(blocos[k].front_dir);
    }

    // Libera o vetor que armazenava os blocos
    free(blocos);
}


// Calcula um único bloco da matriz LCS
static int calcular_bloco(struct Bloco *bloco, const struct Sequencia *A, const struct Sequencia *B, const struct Bloco *bloco_cima, const struct Bloco *bloco_esq) {

    int h = bloco->altura;
    int l = bloco->largura;

    // Número de colunas da matriz local
    // A coluna extra é utilizada para armazenar a fronteira esquerda
    int colunas = l + 1;

    // Aloca a matriz temporária do bloco e inicializa suas posições com zero
    mtype *matriz = calloc(
        (size_t)(h + 1) * (l + 1),
        sizeof(mtype)
    );

    if (matriz == NULL) {
        fprintf(stderr,
            "Erro de alocacao: bloco (%d, %d), altura=%d, largura=%d\n",
            bloco->inicio_i,
            bloco->inicio_j,
            bloco->altura,
            bloco->largura);
            return -1;
    }
        


    // Inicializa a primeira linha com a fronteira inferior do bloco que está acima do bloco atual.
    if (bloco_cima != NULL) {

        for (int j = 0; j <= l; j++) {
            matriz[j] = bloco_cima->front_baixo[j];
        }
    }


    // Inicializa a primeira coluna com a fronteira direita do bloco que está à esquerda do bloco atual.
    if (bloco_esq != NULL) {

        for (int i = 1; i <= h; i++) {
            matriz[i * colunas] = bloco_esq->front_dir[i];
        }
    }


    // Calcula as células internas do bloco
    for (int i = 1; i <= h; i++) {

        for (int j = 1; j <= l; j++) {

            // Converte a posição (i,j) da matriz local para um índice do vetor que armazena a matriz.
            int indice = i * colunas + j;

            // Obtém as posições correspondentes nas sequências A e B.
            int pos_A = bloco->inicio_j + j - 2;
            int pos_B = bloco->inicio_i + i - 2;

            // Compara os caracteres das duas sequências.
            if (A->dados[pos_A] == B->dados[pos_B]) {

                // Se forem iguais, soma 1 ao valor da diagonal superior esquerda.
                matriz[indice] =
                    matriz[(i - 1) * colunas + (j - 1)] + 1;

            } else {

                // Se forem diferentes, compara o valor de cima com o valor da esquerda.
                mtype cima = matriz[(i - 1) * colunas + j];
                mtype esquerda = matriz[i * colunas + (j - 1)];

                // Armazena o maior dos dois valores
                matriz[indice] = (cima > esquerda) ? cima : esquerda;
            }
        }
    }


    // Copia a última linha (fronteira inferior  do bloco)
    for (int j = 0; j <= l; j++) {
        bloco->front_baixo[j] = matriz[h * colunas + j];
    }


    // Copia a última coluna (fronteira direita)
    for (int i = 0; i <= h; i++) {
        bloco->front_dir[i] = matriz[i * colunas + l];
    }

    // Libera a matriz temporária
    free(matriz);

    return 0;

}


// Implementação sequencial do wavefront por blocos
int lcs_wavefront(const struct Sequencia *A, const struct Sequencia *B, int tam_bloco) {

    if (A == NULL || B == NULL ||
        A->dados == NULL || B->dados == NULL ||
        tam_bloco <= 0) {

        fprintf(stderr, "Parâmetros inválidos\n");
        return -1;
    }

    int rank, num_processos;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processos);

    int tamA = A->tam;
    int tamB = B->tam;

    // Se alguma sequência estiver vazia, a LCS tem tamanho zero
    if (tamA == 0 || tamB == 0)
        return 0;


    // Calcula quantos blocos serão necessários em cada dimensão
    int num_blocos_colunas =
        (tamA + tam_bloco - 1) / tam_bloco;

    int num_blocos_linhas =
        (tamB + tam_bloco - 1) / tam_bloco;

    // Calcula a quantidade total de blocos
    int qtde = num_blocos_linhas * num_blocos_colunas;

    // Aloca o vetor de blocos e inicializa com zero.
    struct Bloco *blocos = calloc(qtde, sizeof(struct Bloco));

    if (blocos == NULL) {
        fprintf(stderr, "Erro ao alocar os blocos\n");
        return -1;
    }

    // Define as dimensões e aloca as fronteiras de todos os blocos
    for (int i = 0; i < num_blocos_linhas; i++) {

        for (int j = 0; j < num_blocos_colunas; j++) {

            // Converte as coordenadas do bloco em um índice do vetor
            int indice = i * num_blocos_colunas + j;

            // Obtém um ponteiro para o bloco atual
            struct Bloco *bloco = &blocos[indice];

            // Define a primeira linha da matriz global calculada pelo bloco
            bloco->inicio_i = i * tam_bloco + 1;

            // Define a primeira coluna da matriz global calculada pelo bloco
            bloco->inicio_j = j * tam_bloco + 1;

            // Calcula a altura e a largura do bloco
            bloco->altura = tamB - i * tam_bloco;
            bloco->largura = tamA - j * tam_bloco;

            // Limita a altura e a largura ao tamanho máximo do bloco
            if (bloco->altura > tam_bloco)
                bloco->altura = tam_bloco;

            if (bloco->largura > tam_bloco)
                bloco->largura = tam_bloco;


            // Aloca espaço para a fronteira inferior e a fronteira direita
            bloco->front_baixo = malloc((bloco->largura + 1) * sizeof(mtype));

            bloco->front_dir = malloc((bloco->altura + 1) * sizeof(mtype));


            if (bloco->front_baixo == NULL ||
                bloco->front_dir == NULL) {

                fprintf(stderr, "Erro ao alocar fronteiras\n");

                libera_blocos(blocos, qtde);

                return -1;
            }
        }
    }

    // Calcula o número total de diagonais
    int total_diagonais = num_blocos_linhas + num_blocos_colunas - 1;

    // Percorre cada diagonal
    for (int diag = 0; diag < total_diagonais; diag++) {

        int erro_local = 0;

        // Percorre as linhas para encontrar os blocos da diagonal atual
        // Cada processo calcula seus blocos usando OpenMP
        #pragma omp parallel for reduction(|:erro_local)
        for (int i = 0; i < num_blocos_linhas; i++) {

            // Calcula a coluna do bloco pertencente à diagonal atual
            int j = diag - i;

            // Ignora posições que não correspondem a blocos existentes
            if (j < 0 || j >= num_blocos_colunas)
                continue;

            // Calcula o índice do bloco atual no vetor.
            int indice = i * num_blocos_colunas + j;

            struct Bloco *cima = NULL;
            struct Bloco *esquerda = NULL;

            // Identifica o bloco acima, caso exista.
            if (i > 0) {
                cima = &blocos[(i - 1) * num_blocos_colunas + j];
            }

            // Identifica o bloco à esquerda, caso exista.
            if (j > 0) {
                esquerda = &blocos[i * num_blocos_colunas + (j - 1)];
            }

            int dono = indice % num_processos;

            // Calcula o bloco atual utilizando as fronteiras dos blocos acima e à esquerda
            if (dono == rank) {
                if (calcular_bloco(&blocos[indice], A, B, cima, esquerda) != 0) {
                     fprintf(stderr, "Processo %d falhou no bloco (%d, %d), indice=%d\n", rank, i, j, indice);
                    erro_local = 1;
                }
            }
            
        }

        // Verifica se algum processo teve erro
        int erro_global = 0;

        // Permite combinar valores de todos os processos e disponibilizar o resultado para todos
        MPI_Allreduce(&erro_local, &erro_global, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        
        if (erro_global) {
            if (rank == 0)
                fprintf(stderr, "Erro ao calcular bloco\n");
            libera_blocos(blocos, qtde);
            return -1;
        }

        // Compartilha as fronteiras dos blocos da diagonal
        for (int i = 0; i < num_blocos_linhas; i++) {
            int j = diag - i;

            if (j < 0 || j >= num_blocos_colunas)
                continue;
            
            // Transforma a posição do bloco em um índice linear
            int indice = i * num_blocos_colunas + j;
            // Define qual processo é responsável por calcular o bloco
            int dono = indice % num_processos;

            // Broadcast: faz uma transmissão de um processo para todos os outros processos do comunicador
            MPI_Bcast(blocos[indice].front_baixo, blocos[indice].largura + 1,
                      MPI_UNSIGNED_SHORT, dono, MPI_COMM_WORLD);
            
            MPI_Bcast(blocos[indice].front_dir, blocos[indice].altura + 1,
                      MPI_UNSIGNED_SHORT, dono, MPI_COMM_WORLD);
        }
    }

    // Obtém o resultado na última célula do último bloco

    struct Bloco *ultimo = &blocos[qtde - 1];

    mtype resultado = ultimo->front_baixo[ultimo->largura];

    // Libera a memória utilizada pelos blocos
    libera_blocos(blocos, qtde);

    // Retorna o comprimento da maior subsequência comum.
    return (int)resultado;
}