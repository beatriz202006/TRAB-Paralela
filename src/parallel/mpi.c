#include "lcs.h"

/* Distribui as linhas de blocos entre os processos MPI; 
   linhas próximas ficam agrupadas no mesmo processo
 */
int dono_bloco(int linha, int num_blocos_linhas, int num_processos) {
    return (int)(((long long)linha * num_processos) / num_blocos_linhas);
}

/* Transfere a fronteira inferior dos blocos para o processo responsável pela linha de blocos seguinte */
int sincronizar_diagonal(struct Bloco *blocos, int num_blocos_linhas, int num_blocos_colunas, int diag, int num_processos) {
    int rank;
    int erro_local = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < num_blocos_linhas; i++) {
        int j = diag - i;

        if (j < 0 || j >= num_blocos_colunas)
            continue;

        // Não existe bloco abaixo da última linha
        if (i + 1 >= num_blocos_linhas)
            continue;

        int indice = i * num_blocos_colunas + j;

        int origem = dono_bloco(
            i, num_blocos_linhas, num_processos
        );

        int destino = dono_bloco(
            i + 1, num_blocos_linhas, num_processos
        );

        // Se os dois blocos pertencem ao mesmo processo, a fronteira já está disponível localmente
        if (origem == destino)
            continue;

        int quantidade = blocos[indice].largura + 1;

        if (rank == origem) {
            if (MPI_Send(blocos[indice].front_baixo, quantidade, MPI_UNSIGNED_SHORT, destino, 10, MPI_COMM_WORLD) != MPI_SUCCESS) {
                erro_local = 1;
            }
        } else if (rank == destino) {
            if (MPI_Recv(blocos[indice].front_baixo, quantidade, MPI_UNSIGNED_SHORT, origem, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                erro_local = 1;
            }
        }
    }

    return erro_local;
}