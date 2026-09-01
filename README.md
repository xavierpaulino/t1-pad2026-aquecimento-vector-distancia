# T1: Aquecimento — Distância Quadrática entre Vetores

Este repositório contém as implementações base e otimizada e a infraestrutura experimental utilizada na avaliação de desempenho do cálculo da distância quadrática entre um vetor de referência e um conjunto de vetores.

Para um vetor de referência `q` e um conjunto de `N` vetores `x_i`, cada um com `T` elementos reais, calcula-se:

$$
D(q,x_i) = \sum_{j=0}^{T-1}(q_j-x_{ij})^2, \qquad i=0,\ldots,N-1.
$$

A versão otimizada utiliza SIMD AVX, desenrolamento do laço, quatro acumuladores vetoriais independentes e qualificadores `restrict`, mantendo a execução em um único fluxo. A região cronometrada contém somente o cálculo das `N` distâncias e o armazenamento dos resultados.

## 1. Requisitos

- GNU/Linux x86-64;
- GCC com suporte a C++20;
- GNU Make;
- Python 3;
- `taskset`;
- NumPy, pandas e Matplotlib.

O benchmark foi compilado com o GCC do sistema (`/usr/bin/g++`) usando `-O3 -march=native`, sem `-ffast-math`.

## 2. Instalação e execução

Clone o repositório e entre no diretório:

```bash
git clone https://github.com/xavierpaulino/t1-pad2026-aquecimento-distancia-euclidiana.git
cd t1-pad2026-aquecimento-distancia-euclidiana
```

Instale as dependências Python e selecione o compilador:

```bash
python3 -m pip install -r requirements.txt
export CXX=/usr/bin/g++
```

Compile e execute os testes de corretude:

```bash
make clean
make release
make test
```

Uma execução correta do teste produz:

```text
SELF_TEST_OK
```

Para executar a interface padronizada, informe o tamanho do vetor `T` como primeiro argumento posicional:

```bash
./build/vector_distance 1000
```

A saída padrão contém exclusivamente uma linha no formato:

```text
xavier, 1000, <tempo_ms>
```

O terceiro campo corresponde ao tempo de execução em milissegundos. Caso sejam utilizados parâmetros adicionais, eles devem ser informados após `T`.

Para executar a calibração, a comparação entre as versões, a análise dos dados e a geração dos gráficos:

```bash
python3 scripts/calibrate_n.py
./scripts/run_comparison_experiments.sh
python3 scripts/analyze_comparison.py
python3 scripts/plot_comparison.py
```

A comparação completa também pode ser executada por:

```bash
make part2
```

Opcionalmente, o relatório de vetorização do GCC pode ser gerado com:

```bash
make vectorization
```

O arquivo resultante é armazenado em `build/vectorization.txt`.

## 3. Configuração experimental

A configuração selecionada pela calibração e utilizada na avaliação atual é:

```text
N = 16384
CPU lógico = 2
seed = 42
T = 32, 64, 128, 256, 512, 1024, 2048, 4096
```

Cada combinação entre implementação e tamanho do vetor foi medida 30 vezes, distribuídas em cinco blocos de seis repetições, com duas execuções de aquecimento por combinação em cada bloco. A ordem das combinações é pseudoaleatória e determinística.

A temporização utiliza `CLOCK_MONOTONIC_RAW`. Na calibração atual, a mediana do kernel para `T=32` foi de `0,441797 ms`, a mediana entre leituras consecutivas do relógio foi de `28 ns` e a razão temporizador/kernel foi de `0,006338%`, abaixo do limite de aceitação de `1%`.

A configuração e os dados de calibração são preservados em:

```text
config/experiment.conf
config/calibration.json
```

## 4. Resultados

A versão otimizada apresentou menor tempo mediano em todos os tamanhos avaliados. O fator de aceleração variou aproximadamente entre `1,22×` e `1,73×`, com o maior ganho em `T=1024`.

| T | Base (ms) | Otimizada (ms) | Aceleração | Redução |
|---:|---:|---:|---:|---:|
| 32 | 0,478 | 0,392 | 1,22× | 17,8% |
| 64 | 1,047 | 0,805 | 1,30× | 23,1% |
| 128 | 2,160 | 1,503 | 1,44× | 30,4% |
| 256 | 4,708 | 2,907 | 1,62× | 38,3% |
| 512 | 9,373 | 5,817 | 1,61× | 37,9% |
| 1024 | 19,227 | 11,092 | **1,73×** | **42,3%** |
| 2048 | 40,187 | 27,750 | 1,45× | 30,9% |
| 4096 | 77,485 | 51,536 | 1,50× | 33,5% |

Os dados completos da comparação estão disponíveis em `data/part2/`, e os gráficos correspondentes em `results/part2/`.

## 5. Artefatos de reprodutibilidade

Os principais arquivos utilizados para reprodução e auditoria da avaliação são:

```text
config/experiment.conf
config/calibration.json
data/part2/raw_measurements_comparison.csv
data/part2/measurements_comparison_with_metrics.csv
data/part2/summary_comparison.csv
data/part2/comparison_table.csv
system/system_info.txt
system/experiment_parameters.txt
system/part2_experiment_parameters.txt
```

## 6. Plataforma experimental

Os resultados foram obtidos em um Lenovo ThinkPad T530 com Intel Core i5-3320M @ 2.60 GHz, arquitetura x86-64, dois núcleos físicos, quatro CPUs lógicos e suporte a AVX. O ambiente utilizou GCC 10.5.0 e Ubuntu 24.04.4 LTS. As execuções experimentais foram fixadas no CPU lógico 2.

## 7. Relatório

O relatório do trabalho está disponível no arquivo:

```text
PAD_2026___T1___Relatorio_2.pdf
```

## Autor

**Xavier Paulino Sebastião**  
Processamento de Alto Desempenho — 2026
