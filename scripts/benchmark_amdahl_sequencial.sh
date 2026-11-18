#!/bin/bash

set -euo pipefail
export LC_ALL=C

REPETICOES=20
RESULTADO="resultados/benchmark_amdahl_sequencial.csv"

mkdir -p resultados

echo "repeticao,preparacao,calculo,finalizacao,total,lcs" > "$RESULTADO"

for i in $(seq 1 "$REPETICOES"); do
    echo "Execução $i/$REPETICOES..."

    SAIDA=$(./lcs_seq)

    PREPARACAO=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo de preparação" {gsub(/ s/, "", $2); print $2}')
    CALCULO=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo de cálculo" {gsub(/ s/, "", $2); print $2}')
    FINALIZACAO=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo de finalização" {gsub(/ s/, "", $2); print $2}')
    TOTAL=$(echo "$SAIDA" | awk -F': ' '$1 == "Tempo total" {gsub(/ s/, "", $2); print $2}')
    LCS=$(echo "$SAIDA" | awk -F': ' '$1 == "Score" {print $2}')

    if [[ "$LCS" != "26055" ]]; then
        echo "ERRO: resultado incorreto na execução $i: $LCS"
        exit 1
    fi

    if [[ -z "$PREPARACAO" || -z "$CALCULO" ||
          -z "$FINALIZACAO" || -z "$TOTAL" ]]; then
        echo "ERRO: não foi possível extrair todos os tempos."
        exit 1
    fi

    echo "$i,$PREPARACAO,$CALCULO,$FINALIZACAO,$TOTAL,$LCS" >> "$RESULTADO"

    echo "  Total: ${TOTAL}s | LCS: $LCS"
done

echo ""
echo "Benchmark concluído!"
echo "Resultados: $RESULTADO"