#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

BLOCO=128
REPETICOES=20
CSV="resultados/benchmark_tamanhos.csv"
LOGS="resultados/logs_tamanhos"

mkdir -p resultados "$LOGS"

echo "tipo,tamanho,mpi,omp,repeticao,tempo_s,resultado" > "$CSV"

CONFIGURACOES=(
    "1 1"
    "1 2"
    "1 4"
    "1 8"
    "2 1"
    "2 2"
    "2 4"
    "4 1"
)

executar() {
    local tipo="$1"
    local tamanho="$2"
    local mpi="$3"
    local omp="$4"
    local repeticao="$5"

    local esperado
    case "$tamanho" in
        10000) esperado=3235 ;;
        25000) esperado=8122 ;;
        50000) esperado=16274 ;;
        *) echo "Tamanho desconhecido"; exit 1 ;;
    esac

    local A="inputs/generated/fileA_${tamanho}.in"
    local B="inputs/generated/fileB_${tamanho}.in"

    local prefixo="${LOGS}/${tipo}_${tamanho}_mpi${mpi}_omp${omp}_r${repeticao}"
    local saida="${prefixo}.out"
    local erro="${prefixo}.err"
    local tempo="${prefixo}.time"

    local status=0

    if [[ "$tipo" == "sequencial" ]]; then
        cp "$A" fileA.in
        cp "$B" fileB.in

        if /usr/bin/time -f '%e' -o "$tempo" \
            ./lcs_seq > "$saida" 2> "$erro"; then
            status=0
        else
            status=$?
        fi
    else
        if /usr/bin/time -f '%e' -o "$tempo" \
            mpirun --bind-to none -np "$mpi" \
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
        echo "ERRO: tamanho=$tamanho tipo=$tipo MPI=$mpi OMP=$omp"
        echo "Resultado: ${resultado:-não encontrado}"
        echo "Confira $saida e $erro"
        exit 1
    fi

    echo "$tipo,$tamanho,$mpi,$omp,$repeticao,$segundos,$resultado" >> "$CSV"

    echo "OK: $tipo | N=$tamanho | MPI=$mpi | OMP=$omp | repetição=$repeticao | ${segundos}s"
}

for tamanho in 10000 25000 50000; do
    echo
    echo "========== TAMANHO $tamanho =========="

    for ((r=1; r<=REPETICOES; r++)); do
        executar "sequencial" "$tamanho" 0 1 "$r"

        mapfile -t ORDEM < <(printf '%s\n' "${CONFIGURACOES[@]}" | shuf)

        for configuracao in "${ORDEM[@]}"; do
            read -r mpi omp <<< "$configuracao"
            executar "paralelo" "$tamanho" "$mpi" "$omp" "$r"
        done
    done
done

echo
echo "Benchmark concluído!"
echo "CSV: $CSV"