#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcs.h"

// Lê uma sequência de caracteres de um arquivo
static int ler_sequencia(const char *nome_arquivo, struct Sequencia *seq) {
    FILE *arquivo = fopen(nome_arquivo, "r");

    if (arquivo == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo: %s\n", nome_arquivo);
        return -1;
    }

    // Encontra o tamanho do arquivo
    if (fseek(arquivo, 0, SEEK_END) != 0) {
        fclose(arquivo);
        return -1;
    }

    long tamanho_arquivo = ftell(arquivo);

    if (tamanho_arquivo < 0 || fseek(arquivo, 0, SEEK_SET) != 0) {
        fclose(arquivo);
        return -1;
    }

    seq->dados = malloc((size_t)tamanho_arquivo + 1);

    if (seq->dados == NULL) {
        fprintf(stderr, "Erro ao alocar memória para a sequência.\n");
        fclose(arquivo);
        return -1;
    }

    // Lê o conteúdo, ignorando quebras de linha 
    int caractere;
    int i = 0;

    while ((caractere = fgetc(arquivo)) != EOF) {
        if (caractere != '\n' && caractere != '\r') {
            seq->dados[i++] = (char)caractere;
        }
    }

    seq->dados[i] = '\0';
    seq->tam = i;

    fclose(arquivo);
    return 0;
}

int main(int argc, char *argv[]) {

    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        fprintf(stderr, "Erro ao inicializar MPI.\n");
        return EXIT_FAILURE;
    }
    
    int rank, num_processos;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processos);

    // Verifica os argumentos
    if (argc != 4) {

        if (rank == 0) {
             fprintf(stderr, "Uso: %s <arquivo_A> <arquivo_B> <tamanho_bloco>\n", argv[0]);
        }

        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int tam_bloco = atoi(argv[3]);

    if (tam_bloco <= 0) {

        if (rank == 0) {
            fprintf(stderr, "Tamanho de bloco inválido.\n");
        }

        MPI_Finalize();
        return EXIT_FAILURE;
    }

    struct Sequencia A = {NULL, 0};
    struct Sequencia B = {NULL, 0};

    // Lê as duas sequências
   if (ler_sequencia(argv[1], &A) != 0 || ler_sequencia(argv[2], &B) != 0) {

        if (rank == 0)
            fprintf(stderr, "Erro ao ler as sequências.\n");

        free(A.dados);
        free(B.dados);

        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Executa o algoritmo LCS
    int resultado = lcs_wavefront(&A, &B, tam_bloco);

   if (rank == 0 && resultado >= 0) {
        printf("LCS: %d\n", resultado);
    }


    // Libera a memória das sequências
    free(A.dados);
    free(B.dados);

    MPI_Finalize();

    if (resultado < 0)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}