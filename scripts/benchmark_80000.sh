#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

A="inputs/generated/fileA_80000.in"
B="inputs/generated/fileB_80000.in"
BLOCO=128
REPETICOES=20
ESPERADO=26055

CSV="resultados/benchmark_80000.csv"
LOGS="resultados/logs_80000"

mkdir -p resultados "$LOGS"

# Verifica os arquivos e executáveis
for arquivo in "$A" "$B" ./lcs ./lcs_seq; do
    if [[ ! -e "$arquivo" ]]; then
        echo "Arquivo não encontrado: $arquivo"
        exit 1
    fi
done

# O executável sequencial lê estes nomes fixos
cp "$A" fileA.in
cp "$B" fileB.in

echo "tipo,mpi,omp,repeticao,tempo_s,resultado" > "$CSV"

executar() {
    local tipo="$1"
    local mpi="$2"
    local omp="$3"
    local repeticao="$4"

    local prefixo="${LOGS}/${tipo}_mpi${mpi}_omp${omp}_r${repeticao}"
    local saida="${prefixo}.out"
    local erro="${prefixo}.err"
    local tempo="${prefixo}.time"
    local status=0

    echo "Executando: $tipo | MPI=$mpi | OMP=$omp | repetição=$repeticao"

    if [[ "$tipo" == "sequencial" ]]; then
        if /usr/bin/time -f '%e' -o "$tempo" \
            ./lcs_seq > "$saida" 2> "$erro"; then
            status=0
        else
            status=$?
        fi
    else
        if /usr/bin/time -f '%e' -o "$tempo" \
            env OMP_NUM_THREADS="$omp" OMP_DYNAMIC=FALSE \
            mpirun --bind-to none -np "$mpi" \
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

    if [[ "$status" -ne 0 || "$resultado" != "$ESPERADO" ]]; then
        echo "ERRO na execução!"
        echo "Código de saída: $status"
        echo "Resultado obtido: ${resultado:-não encontrado}"
        echo "Confira: $saida e $erro"
        exit 1
    fi

    echo "$tipo,$mpi,$omp,$repeticao,$segundos,$resultado" >> "$CSV"
}

# Configurações MPI x OpenMP
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

for ((r=1; r<=REPETICOES; r++)); do
    echo
    echo "========== RODADA $r/$REPETICOES =========="

    executar "sequencial" 0 1 "$r"

    # Embaralha a ordem das configurações nesta rodada
    mapfile -t ORDEM < <(printf '%s\n' "${CONFIGURACOES[@]}" | shuf)

    for configuracao in "${ORDEM[@]}"; do
        read -r mpi omp <<< "$configuracao"
        executar "paralelo" "$mpi" "$omp" "$r"
    done
done

echo
echo "Benchmark concluído!"
echo "Resultados salvos em: $CSV"