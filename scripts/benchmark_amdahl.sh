#!/bin/bash

set -euo pipefail
export LC_ALL=C

ENTRADA_A="inputs/generated/fileA_80000.in"
ENTRADA_B="inputs/generated/fileB_80000.in"
BLOCO=128
REPETICOES=20
RESULTADO="resultados/benchmark_amdahl.csv"

mkdir -p resultados

echo "repeticao,preparacao,calculo,comunicacao,total,lcs" > "$RESULTADO"

export OMP_NUM_THREADS=1
export OMP_DYNAMIC=FALSE

for i in $(seq 1 "$REPETICOES"); do
    echo "Execução $i/$REPETICOES..."

    SAIDA=$(mpirun -np 1 --bind-to none ./lcs \
        "$ENTRADA_A" "$ENTRADA_B" "$BLOCO")

    PREPARACAO=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo de preparação" {print $2}')
    CALCULO=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo de cálculo" {print $2}')
    COMUNICACAO=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo de comunicação" {print $2}')
    TOTAL=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo total" {print $2}')
    LCS=$(echo "$SAIDA" | awk -F': ' '$1 == "LCS" {print $2}')

    if [[ "$LCS" != "26055" ]]; then
        echo "ERRO: resultado incorreto na execução $i: $LCS"
        exit 1
    fi

    if [[ -z "$PREPARACAO" || -z "$CALCULO" ||
          -z "$COMUNICACAO" || -z "$TOTAL" ]]; then
        echo "ERRO: não foi possível extrair todos os tempos."
        exit 1
    fi

    echo "$i,$PREPARACAO,$CALCULO,$COMUNICACAO,$TOTAL,$LCS" \
        >> "$RESULTADO"

    echo "  Total: ${TOTAL}s | Cálculo: ${CALCULO}s"
done

echo ""
echo "Benchmark concluído!"
echo "Resultados salvos em: $RESULTADO"