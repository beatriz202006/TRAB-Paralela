#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lcs.h"

// Libera os buffers utilizados pelas threads
static void libera_buffers(mtype **buffers, int quantidade) {

    if (buffers == NULL)
        return;

    for (int i = 0; i < quantidade; i++) {
        free(buffers[i]);
    }

    free(buffers);
}


// Calcula o LCS utilizando wavefront por blocos
int lcs_wavefront(
    const struct Sequencia *A,
    const struct Sequencia *B,
    int tam_bloco
) {

    int rank;
    int num_processos;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processos);

    // Temporizadores das diferentes etapas
    double tempo_preparacao = 0.0;
    double tempo_calculo = 0.0;
    double tempo_comunicacao = 0.0;
    double inicio_fase = MPI_Wtime();
    double inicio_total = inicio_fase;


    int tamA = A->tam;
    int tamB = B->tam;


    // Calcula a quantidade de blocos da matriz
    int num_blocos_colunas = (tamA + tam_bloco - 1) / tam_bloco;

    int num_blocos_linhas = (tamB + tam_bloco - 1) / tam_bloco;

    int qtde = num_blocos_linhas * num_blocos_colunas;


    // Aloca os blocos
    struct Bloco *blocos = calloc(qtde, sizeof(struct Bloco));

    if (blocos == NULL) {
        fprintf(stderr, "Erro ao alocar os blocos.\n");
        return -1;
    }

    // Inicializa os blocos
    for (int i = 0; i < num_blocos_linhas; i++) {

        for (int j = 0; j < num_blocos_colunas; j++) {

            int indice = i * num_blocos_colunas + j;

            blocos[indice].inicio_i = i * tam_bloco + 1;
            blocos[indice].inicio_j = j * tam_bloco + 1;

            // Define a altura do bloco
            int altura = tamB - i * tam_bloco;

            if (altura > tam_bloco)
                altura = tam_bloco;

            blocos[indice].altura = altura;

            // Define a largura do bloco
            int largura = tamA - j * tam_bloco;

            if (largura > tam_bloco)
                largura = tam_bloco;

            blocos[indice].largura = largura;

            // Aloca as fronteiras do bloco
            blocos[indice].front_baixo = malloc((size_t)(largura + 1) * sizeof(mtype));

            blocos[indice].front_dir = malloc((size_t)(altura + 1) * sizeof(mtype));

            if (blocos[indice].front_baixo == NULL || blocos[indice].front_dir == NULL) {
                fprintf(stderr, "Erro ao alocar fronteiras do bloco (%d, %d).\n", i, j);
                libera_blocos(blocos, qtde);
                return -1;
            }
        }
    }

    // Define a quantidade máxima de threads OpenMP
    int num_threads = omp_get_max_threads();

    if (num_threads < 1)
        num_threads = 1;

    // Calcula o tamanho necessário para cada buffer
    size_t lado = (size_t)tam_bloco + 1;

    int erro_buffers = 0;

    mtype **buffers = calloc((size_t)num_threads, sizeof(mtype *));

    if (buffers == NULL || lado > SIZE_MAX / lado) {
        erro_buffers = 1;
    }

    size_t capacidade = 0;

    if (!erro_buffers) {
        capacidade = lado * lado;

        if (capacidade > SIZE_MAX / sizeof(mtype)) {
            erro_buffers = 1;
        }
    }

    // Aloca um buffer para cada thread
    if (!erro_buffers) {
        for (int t = 0; t < num_threads; t++) {

            buffers[t] = malloc(capacidade * sizeof(mtype));

            if (buffers[t] == NULL) {

                erro_buffers = 1;

                break;
            }
        }
    }

    // Finaliza a medição da preparação local
    tempo_preparacao = MPI_Wtime() - inicio_fase;

    // Mede a comunicação da verificação das alocações
    inicio_fase = MPI_Wtime();

    // Verifica se todos os processos conseguiram alocar os buffers
    int erro_buffers_global = 0;

    MPI_Allreduce(&erro_buffers, &erro_buffers_global, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    tempo_comunicacao += MPI_Wtime() - inicio_fase;

    if (erro_buffers_global) {

        if (rank == 0)
            fprintf(stderr, "Erro ao alocar os buffers das threads.\n");

        libera_buffers(buffers, num_threads);
        libera_blocos(blocos, qtde);

        return -1;
    }


    // Percorre as diagonais da matriz de blocos
    int total_diagonais =
        num_blocos_linhas + num_blocos_colunas - 1;

    for (int diag = 0; diag < total_diagonais; diag++) {

        // Mede o cálculo paralelo da diagonal
        inicio_fase = MPI_Wtime();

        int erro_local = calcular_diagonal(blocos, num_blocos_linhas, num_blocos_colunas, diag, rank, num_processos, A, B, buffers);

        tempo_calculo += MPI_Wtime() - inicio_fase;

        // Verifica erros de cálculo entre os processos
        int erro_global = 0;

        // Mede a comunicação da verificação de erros de cálculo
        inicio_fase = MPI_Wtime();
        MPI_Allreduce(&erro_local, &erro_global, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        tempo_comunicacao += MPI_Wtime() - inicio_fase;

        if (erro_global) {
            libera_buffers(buffers, num_threads);
            libera_blocos(blocos, qtde);
            return -1;
        }

        // Mede a sincronização das fronteiras entre processos
        inicio_fase = MPI_Wtime();
        // Sincroniza as fronteiras entre os processos MPI
        int erro_sync = sincronizar_diagonal(blocos, num_blocos_linhas, num_blocos_colunas, diag, num_processos);

        // Verifica erros de sincronização
        MPI_Allreduce(&erro_sync, &erro_global, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        tempo_comunicacao += MPI_Wtime() - inicio_fase;

        if (erro_global) {
            libera_buffers(buffers, num_threads);
            libera_blocos(blocos, qtde);
            return -1;
        }
    }

    // Identifica o processo responsável pelo último bloco
    int dono_resultado = dono_bloco(num_blocos_linhas - 1, num_blocos_linhas, num_processos);

    // Obtém o resultado no processo que possui o último bloco
    mtype resultado = 0;

    if (rank == dono_resultado) {
        struct Bloco *ultimo = &blocos[(num_blocos_linhas - 1) * num_blocos_colunas + (num_blocos_colunas - 1)];

        resultado = ultimo->front_baixo[
            ultimo->largura
        ];
    }

    // Mede a comunicação do resultado final
    inicio_fase = MPI_Wtime();
    // Compartilha o resultado final com todos os processos
    MPI_Bcast(&resultado, 1, MPI_UNSIGNED_SHORT, dono_resultado, MPI_COMM_WORLD);
    tempo_comunicacao += MPI_Wtime() - inicio_fase;
    double tempo_total = MPI_Wtime() - inicio_total;

    // Tempos locais de cada processo
    double tempos_locais[4] = {
        tempo_preparacao,
        tempo_calculo,
        tempo_comunicacao,
        tempo_total
    };

    // Maiores tempos observados entre os processos
    double tempos_maximos[4] = {0.0, 0.0, 0.0, 0.0};

    MPI_Reduce(
        tempos_locais,
        tempos_maximos,
        4,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD
    );

    // Exibe os tempos apenas no processo raiz
    if (rank == 0) {
        printf("Tempo de preparação: %.6f s\n", tempos_maximos[0]);
        printf("Tempo de cálculo: %.6f s\n", tempos_maximos[1]);
        printf("Tempo de comunicação: %.6f s\n", tempos_maximos[2]);
        printf("Tempo total: %.6f s\n", tempos_maximos[3]);
    }

    // Libera a memória utilizada
    libera_buffers(buffers, num_threads);
    libera_blocos(blocos, qtde);

    return (int)resultado;
}
