# T1: Aquecimento - Distância Quadrática entre Vetores

Este repositório contém as implementações base e otimizada, os scripts experimentais, os resultados e o relatório do Trabalho 1 da disciplina de Processamento de Alto Desempenho (PAD).

Para um vetor de referência `q` e um conjunto de `N` vetores `x_i`, cada um com `T` valores reais, é calculada a distância quadrática

```text
D(q, x_i) = Σ_j (q_j - x_ij)²
```

para todos os vetores do conjunto. A região cronometrada contém exclusivamente o cálculo das distâncias e o armazenamento dos resultados. Alocação de memória, geração e inicialização dos dados, aquecimento, cálculo de checksum, escrita de arquivos e impressão ficam fora do intervalo medido.

A versão final mantém duas implementações no mesmo executável:

- `baseline`: versão base com laço escalar;
- `optimized`: versão otimizada com SIMD AVX, desenrolamento do laço, quatro acumuladores vetoriais independentes e `__restrict__`.

A comparação final foi realizada com `N = 8192` constante e `T ∈ {32, 64, 128, 256, 512, 1024, 2048, 4096}`.

## 1. Requisitos

O projeto foi desenvolvido para Linux x86-64. São necessários:

- GCC com suporte a C++20;
- GNU Make;
- Python 3;
- `taskset`;
- NumPy;
- pandas;
- Matplotlib.

Ferramentas como `lscpu` e `perf` podem ser utilizadas para caracterização complementar da plataforma.

## 2. Clonar o repositório

```bash
git clone https://github.com/xavierpaulino/t1-pad2026-aquecimento-distancia-euclidiana/
cd t1-pad2026-aquecimento-distancia-euclidiana
```

## 3. Ambiente Python

```bash
conda create -n t0_vector_distance python=3.12 numpy pandas matplotlib -c conda-forge
conda activate t0_vector_distance
```

O benchmark C++ utiliza o GCC do sistema:

```bash
export CXX=/usr/bin/g++
```

## 4. Compilar e verificar a implementação

```bash
make clean
make release
make test
```

O teste de corretude produz:

```text
SELF_TEST_OK
```

O relatório do auto-vetorizador do GCC pode ser gerado com:

```bash
make vectorization
```

O arquivo é gravado em `build/vectorization.txt`.

## 5. Interface de execução exigida

O tamanho `T` é informado como primeiro argumento posicional:

```bash
./build/vector_distance 1000
```

Para uma execução válida, a única saída em `stdout` segue o formato:

```text
xavier, 1000, tempo_ms
```

O terceiro campo representa o tempo do kernel em milissegundos. A interface posicional utiliza a versão otimizada e mantém `N = 8192` fixo.

Exemplos para os tamanhos avaliados:

```bash
./build/vector_distance 32
./build/vector_distance 64
./build/vector_distance 128
./build/vector_distance 256
./build/vector_distance 512
./build/vector_distance 1024
./build/vector_distance 2048
./build/vector_distance 4096
```

## 6. Reproduzir a comparação entre as versões

A configuração correspondente aos resultados finais está registrada em `config/experiment.conf`.

```bash
./scripts/run_comparison_experiments.sh
python3 scripts/analyze_comparison.py
python3 scripts/plot_comparison.py
```

A mesma sequência pode ser executada por:

```bash
make part2
```

A execução produz:

```text
data/part2/raw_measurements_comparison.csv
data/part2/measurements_comparison_with_metrics.csv
data/part2/summary_comparison.csv
data/part2/comparison_table.csv
```

Os gráficos comparativos são gravados em `results/part2/`.

## 7. Resultados finais

O arquivo `data/part2/comparison_table.csv` reúne os principais valores utilizados no relatório. Os gráficos finais estão em `results/part2/`:

- `execution_time_comparison.png` — tempo mediano de execução;
- `vectors_per_second_comparison.png` — vazão de vetores;
- `elements_per_second_comparison.png` — vazão de elementos;
- `ns_per_element_comparison.png` — custo temporal por elemento;
- `execution_time_variability_comparison.png` — distribuição dos tempos;
- `speedup.png` — fator de aceleração da versão otimizada.

A versão otimizada foi mais rápida em todos os tamanhos avaliados. O fator de aceleração observado variou aproximadamente de `1,44` a `1,80 vezes`.

## 8. Estrutura do repositório


## 9. Relatório

O relatório está disponível em:

```text
Relatorio_PAD_T1_2026.pdf
```
