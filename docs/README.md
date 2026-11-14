# LCS — Programação Paralela

Trabalho prático da disciplina de Programação Paralela (UFPR — CI1316), desenvolvido em dupla.

O objetivo é implementar uma versão paralela do algoritmo **Longest Common Subsequence (LCS)** utilizando uma abordagem híbrida com MPI e OpenMP, comparar seu desempenho com uma implementação sequencial e analisar os resultados experimentais.

---

## 1. Objetivo

O problema LCS consiste em encontrar o comprimento da maior subsequência comum entre duas sequências de caracteres, preservando a ordem dos elementos, mas não necessariamente suas posições consecutivas.

A implementação utiliza programação dinâmica para calcular a matriz de pontuação do algoritmo.

O trabalho contempla:

- Implementação sequencial do algoritmo LCS.
- Implementação paralela híbrida utilizando MPI e OpenMP.
- Distribuição do processamento entre processos MPI.
- Paralelização do cálculo dos blocos utilizando OpenMP.
- Comparação de desempenho entre as versões sequencial e paralela.
- Análise de speedup, eficiência, escalabilidade e desvio padrão.

## 2. Estado atual do projeto

A implementação híbrida está desenvolvida e disponível na branch:

`refatoracao-arquivos`

Essa branch contém a implementação MPI + OpenMP mais completa e modularizada, a organização dos arquivos e os primeiros resultados experimentais.

A branch `main` foi preservada separadamente com a primeira implementação inicial.

**Último commit da implementação:**

`c22f3dd` — Implementa versao hibrida completa MPI e OpenMP

### Funcionalidades implementadas

- Cálculo do LCS utilizando programação dinâmica.
- Divisão da matriz em blocos.
- Processamento dos blocos por diagonais (wavefront).
- Distribuição das linhas de blocos entre os processos MPI.
- Paralelização dos blocos de uma mesma diagonal com OpenMP.
- Comunicação das fronteiras entre processos MPI.
- Reutilização de buffers temporários por thread.
- Compartilhamento do resultado final entre os processos MPI.

A implementação foi testada com diferentes quantidades de processos, threads e tamanhos de bloco.

## 3. Estratégia de paralelização

A implementação utiliza uma abordagem híbrida:

### MPI

O MPI é responsável pela distribuição do trabalho entre processos.

A matriz é dividida em blocos de linhas, e cada processo fica responsável por uma parte dessas linhas.

Quando um bloco depende de informações calculadas por outro processo, suas fronteiras são comunicadas utilizando MPI.

### OpenMP

O OpenMP é utilizado para paralelizar o cálculo dos blocos de uma mesma diagonal.

Como os blocos de uma diagonal podem ser calculados independentemente uns dos outros, eles podem ser distribuídos entre as threads.

### Wavefront

O cálculo segue uma estratégia de wavefront por diagonais.

Isso é necessário porque cada célula da matriz LCS depende de valores anteriores: a célula superior, a célula à esquerda e a célula diagonal superior esquerda.

Dessa forma, uma diagonal só é processada depois que as diagonais anteriores necessárias foram calculadas.

### Reutilização de memória

Cada thread OpenMP possui seu próprio buffer temporário para calcular os blocos.

Os buffers são alocados antes do processamento das diagonais e reutilizados durante toda a execução, evitando alocações e liberações de memória a cada bloco.

## 4. Estrutura do projeto

```text
.
├── docs/
│   ├── README.md
│   └── testes.md
│
├── inputs/
│   ├── fileA.in
│   ├── fileB.in
│   └── generated/
│       ├── fileA.in
│       └── fileB.in
│
├── resultados/
│   └── tempos.csv
│
├── scripts/
│   └── gerar_entradas.c
│
└── src/
    ├── sequential/
    │   └── lcs.c
    │
    └── parallel/
        ├── lcs.c
        ├── lcs.h
        ├── main.c
        ├── mpi.c
        ├── openmp.c
        └── wavefront.c
```

### Descrição dos arquivos principais

| Arquivo                    | Responsabilidade                                                       |
| -------------------------- | ---------------------------------------------------------------------- |
| `src/sequential/lcs.c`     | Implementação sequencial do LCS.                                       |
| `src/parallel/main.c`      | Inicialização do MPI e execução do programa paralelo.                  |
| `src/parallel/lcs.c`       | Cálculo dos blocos e gerenciamento das fronteiras.                     |
| `src/parallel/lcs.h`       | Estruturas e declarações compartilhadas.                               |
| `src/parallel/mpi.c`       | Distribuição dos blocos e sincronização MPI.                           |
| `src/parallel/openmp.c`    | Paralelização do cálculo das diagonais com OpenMP.                     |
| `src/parallel/wavefront.c` | Organização do processamento por diagonais e gerenciamento dos blocos. |
| `scripts/gerar_entradas.c` | Geração de entradas para os experimentos.                              |
| `resultados/tempos.csv`    | Tempos das execuções experimentais já realizadas.                      |

## 5. Tecnologias utilizadas

- Linguagem C.
- OpenMP.
- MPI (Open MPI).
- GCC.
- Git e GitHub.
- Sistema operacional Linux (Ubuntu).

## 6. Compilação

Para compilar a implementação híbrida, execute na raiz do projeto:

```bash
mpicc -Wall -Wextra -O3 -fopenmp \
src/parallel/main.c \
src/parallel/lcs.c \
src/parallel/openmp.c \
src/parallel/mpi.c \
src/parallel/wavefront.c \
-o lcs
```

O executável gerado será `lcs`.

### Compilação da versão sequencial

```bash
gcc -O3 src/sequential/lcs.c -o lcs_seq
```

## 7. Execução

A versão híbrida recebe três argumentos:

1. Caminho do primeiro arquivo de entrada.
2. Caminho do segundo arquivo de entrada.
3. Tamanho do bloco utilizado na divisão da matriz.

Exemplo:

```bash
OMP_NUM_THREADS=2 mpirun -np 2 ./lcs \
inputs/generated/fileA.in \
inputs/generated/fileB.in 128
```

Nesse exemplo:

- `OMP_NUM_THREADS=2`: utiliza duas threads OpenMP por processo.
- `-np 2`: inicia dois processos MPI.
- `128`: define o tamanho dos blocos.

O resultado apresentado é o comprimento da maior subsequência comum.

Exemplo de saída:

```text
LCS: 16274
```

## 8. Testes realizados

Foram realizados testes de corretude utilizando entradas com 50.000 caracteres em cada arquivo.

O resultado da versão sequencial foi comparado com o da implementação híbrida.

Resultado esperado:

```text
16274
```

### Testes com diferentes configurações MPI e OpenMP

Foram testadas as seguintes configurações:

| Processos MPI | Threads OpenMP por processo |
| ------------: | --------------------------: |
|             1 |                           1 |
|             1 |                           2 |
|             1 |                           4 |
|             1 |                           8 |
|             2 |                           1 |
|             2 |                           2 |
|             2 |                           4 |
|             4 |                           1 |
|             4 |                           2 |

Todas retornaram o resultado esperado.

### Testes com diferentes tamanhos de bloco

Foram testados os tamanhos:

- 32
- 64
- 128
- 256

Todas as configurações retornaram o resultado esperado.

## 9. Primeiros resultados de desempenho

Foram realizadas 20 execuções por configuração, utilizando entradas de 50.000 caracteres e blocos de tamanho 128.

Os tempos abaixo representam médias das execuções.

| Versão     | Processos MPI | Threads OpenMP | Tempo médio (s) |
| ---------- | ------------: | -------------: | --------------: |
| Sequencial |             1 |              1 |          4,1935 |
| Híbrida    |             1 |              1 |          3,5815 |
| Híbrida    |             1 |              2 |          2,7255 |
| Híbrida    |             1 |              4 |          2,7125 |
| Híbrida    |             1 |              8 |          2,7535 |
| Híbrida    |             2 |              1 |          3,1585 |
| Híbrida    |             2 |              2 |          2,6595 |
| Híbrida    |             2 |              4 |          2,5335 |
| Híbrida    |             4 |              1 |          3,2210 |
| Híbrida    |             4 |              2 |          2,8425 |

A melhor configuração observada até o momento foi de 2 processos MPI e 4 threads OpenMP por processo, com tempo médio de 2,5335 segundos.

Em comparação com a versão sequencial, essa configuração apresentou speedup de aproximadamente 1,655.

Os resultados são preliminares e ainda serão complementados por outros experimentos e análises de escalabilidade.

Os dados individuais das execuções estão disponíveis em `resultados/tempos.csv`.

## 10. Próximas etapas

Ainda devem ser realizadas e documentadas as seguintes atividades:

- [ ] Realizar experimentos de strong scaling.
- [ ] Realizar experimentos de weak scaling.
- [ ] Analisar a influência do tamanho da entrada.
- [ ] Avaliar diferentes tamanhos de bloco no desempenho.
- [ ] Calcular e analisar speedup e eficiência.
- [ ] Analisar o impacto de SMT.
- [ ] Documentar o ambiente experimental: processador, sistema operacional, kernel, compilador e opções de compilação.
- [ ] Avaliar a variabilidade dos tempos e os desvios padrão.
- [ ] Elaborar o relatório final do trabalho.
- [ ] Revisar a implementação e preparar a apresentação/defesa.

## 11. Controle de versão

A implementação híbrida atual está na branch `refatoracao-arquivos`.

Para acessar essa versão:

```bash
git switch refatoracao-arquivos
git pull
```

Para verificar a branch atual:

```bash
git branch
```

A branch `main` permanece separada da branch de desenvolvimento.

---

**Trabalho prático de Programação Paralela — UFPR**

Implementação de LCS utilizando MPI e OpenMP.
