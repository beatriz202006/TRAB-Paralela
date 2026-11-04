#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <tamanho>\n", argv[0]);
        return 1;
    }

    int tamanho = atoi(argv[1]);

    if (tamanho <= 0) {
        printf("O tamanho deve ser positivo.\n");
        return 1;
    }

    FILE *arquivoA = fopen("inputs/generated/fileA.in", "w");
    FILE *arquivoB = fopen("inputs/generated/fileB.in", "w");

    if (arquivoA == NULL || arquivoB == NULL) {
        printf("Erro ao criar os arquivos.\n");

        if (arquivoA != NULL) fclose(arquivoA);
        if (arquivoB != NULL) fclose(arquivoB);

        return 1;
    }

    srand(42);

    for (int i = 0; i < tamanho; i++) {
        fputc('a' + rand() % 26, arquivoA);
        fputc('a' + rand() % 26, arquivoB);
    }

    fclose(arquivoA);
    fclose(arquivoB);

    printf("Entradas geradas: %d caracteres cada.\n", tamanho);

    return 0;
}