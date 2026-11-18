#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

REPETICOES=20
BLOCO=128

CSV="resultados/benchmark_fraca.csv"
LOGS="resultados/logs_fraca"

mkdir -p resultados "$LOGS"

echo "tipo,tamanho,workers,mpi,omp,repeticao,tempo_s,resultado" > "$CSV"

# Tamanho, threads e resultado esperado
CONFIGURACOES=(
    "25000 1 8122"
    "35355 2 11513"
    "50000 4 16274"
    "70711 8 23021"
)

executar() {
    local tipo="$1"
    local n="$2"
    local omp="$3"
    local esperado="$4"
    local repeticao="$5"

    local A="inputs/generated/fileA_${n}.in"
    local B="inputs/generated/fileB_${n}.in"

    local saida="${LOGS}/${tipo}_${n}_omp${omp}_r${repeticao}.out"
    local erro="${LOGS}/${tipo}_${n}_omp${omp}_r${repeticao}.err"
    local tempo="${LOGS}/${tipo}_${n}_omp${omp}_r${repeticao}.time"

    local status=0

    if [[ "$tipo" == "sequencial" ]]; then
        if /usr/bin/time -f '%e' -o "$tempo" \
            ./lcs_seq > "$saida" 2> "$erro"; then
            status=0
        else
            status=$?
        fi
    else
        if /usr/bin/time -f '%e' -o "$tempo" \
            mpirun --bind-to none -np 1 \
            env OMP_NUM_THREADS="$omp" OMP_DYNAMIC=FALSE \
            ./lcs "$A" "$B" "$BLOCO" > "$saida" 2> "$erro"; then
            status=0
        else
            status=$?
        fi
    fi

    local resultado
    resultado=$(grep -Eo '(Score|LCS): [0-9]+' "$saida" |
                tail -n 1 | awk '{print $2}' || true)

    local segundos
    segundos=$(cat "$tempo")

    if [[ "$status" -ne 0 || "$resultado" != "$esperado" ]]; then
        echo "ERRO: tipo=$tipo N=$n OMP=$omp repetição=$repeticao"
        echo "Resultado obtido: ${resultado:-não encontrado}"
        echo "Confira $saida e $erro"
        exit 1
    fi

    echo "$tipo,$n,$omp,1,$omp,$repeticao,$segundos,$resultado" >> "$CSV"

    echo "OK: $tipo | N=$n | OMP=$omp | repetição=$repeticao | ${segundos}s"
}

for configuracao in "${CONFIGURACOES[@]}"; do
    read -r n omp esperado <<< "$configuracao"

    echo
    echo "========== N=$n | WORKERS=$omp =========="

    A="inputs/generated/fileA_${n}.in"
    B="inputs/generated/fileB_${n}.in"

    if [[ ! -f "$A" || ! -f "$B" ]]; then
        echo "Arquivo de entrada não encontrado para N=$n"
        exit 1
    fi

    # A versão sequencial usa nomes fixos
    cp "$A" fileA.in
    cp "$B" fileB.in

    for ((r=1; r<=REPETICOES; r++)); do
        executar "sequencial" "$n" 1 "$esperado" "$r"
        executar "paralelo" "$n" "$omp" "$esperado" "$r"
    done
done

echo
echo "Benchmark de escalabilidade fraca concluído!"
echo "CSV: $CSV"