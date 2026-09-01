# T1: Aquecimento — Distância Quadrática entre Vetores

Este repositório contém as implementações base e otimizada e a infraestrutura experimental utilizadas para avaliar o desempenho do cálculo da distância quadrática entre um vetor de referência e um conjunto de vetores.

Para um vetor de referência `q` e um conjunto de `N` vetores `x_i`, cada um com `T` elementos reais, é calculada:

$$
D(q,x_i)=\sum_{j=0}^{T-1}(q_j-x_{ij})^2,
\qquad i=0,\ldots,N-1.
$$

A versão otimizada utiliza SIMD AVX, desenrolamento do laço, quatro acumuladores vetoriais independentes e qualificadores `restrict`, mantendo a execução em um único fluxo.

A região cronometrada contém exclusivamente o cálculo das `N` distâncias e o armazenamento dos resultados. Alocação de memória, inicialização dos dados, aquecimento, geração de valores, escrita de arquivos, análise estatística e apresentação dos resultados permanecem fora da aferição.

## 1. Requisitos

Os experimentos foram desenvolvidos para GNU/Linux x86-64.

São necessários:

* GCC com suporte a C++20;
* GNU Make;
* Python 3;
* `taskset`;
* NumPy;
* pandas;
* Matplotlib.

Ferramentas complementares utilizadas na caracterização do ambiente incluem `lscpu` e `perf`.

## 2. Clonar o repositório

```bash
git clone https://github.com/xavierpaulino/t1-pad2026-aquecimento-distancia-euclidiana.git
cd t1-pad2026-aquecimento-distancia-euclidiana
```

## 3. Preparar o ambiente

As dependências Python podem ser instaladas diretamente:

```bash
python3 -m pip install -r requirements.txt
```

Também pode ser utilizado um ambiente Conda:

```bash
conda create -n t1_vector_distance python=3.12 numpy pandas matplotlib -c conda-forge
conda activate t1_vector_distance
```

O benchmark C++ utiliza o GCC do sistema:

```bash
export CXX=/usr/bin/g++
```

Essa configuração evita o uso do compilador fornecido pelo ambiente Conda, que pode introduzir requisitos de arquitetura ou dependências diferentes das disponíveis na máquina utilizada nos experimentos.

## 4. Compilar e validar

```bash
make clean
make release
make test
```

Uma execução correta do teste produz:

```text
SELF_TEST_OK
```

A compilação de desempenho utiliza C++20 com:

```text
-O3 -march=native
```

Não é utilizado `-ffast-math`.

O relatório de vetorização do GCC pode ser gerado com:

```bash
make vectorization
```

e é armazenado em:

```text
build/vectorization.txt
```

## 5. Execução do programa

O tamanho do vetor `T` é informado como primeiro argumento da linha de comando:

```bash
./build/vector_distance T
```

Por exemplo:

```bash
./build/vector_distance 1000
```

A saída padrão contém exclusivamente uma linha:

```text
xavier, 1000, 313
```

Os campos correspondem a:

```text
identificação, tamanho_do_vetor, tempo_em_milissegundos
```

A interface posicional executa a versão otimizada, realiza cinco execuções de aquecimento antes da medição e utiliza `N=16384` como valor padrão interno. As execuções de aquecimento não integram o tempo informado.

Esse modo de execução é distinto da configuração utilizada nos experimentos comparativos do relatório, nos quais `N=8192` foi mantido constante para as versões base e otimizada.

## 6. Calibração experimental

A configuração dos experimentos é determinada antes das aferições por:

```bash
python3 scripts/calibrate_n.py
```

A calibração considera o maior tamanho avaliado, `T=4096`, e limita a principal alocação de memória a 25% da `MemAvailable` observada.

O procedimento também seleciona deterministicamente um CPU lógico permitido, considerando a topologia física do processador.

A adequação da temporização é verificada com `CLOCK_MONOTONIC_RAW`, comparando o custo mediano das leituras do relógio com o tempo mediano do kernel para `T=32`.

A configuração utilizada nos experimentos finais foi:

```text
N = 8192
CPU lógico = 2
T máximo = 4096
fração máxima de MemAvailable = 0,25
```

A calibração utilizou 15 aferições, precedidas por três execuções de aquecimento. Para `T=32`, foram registrados aproximadamente `231,5 µs` para a mediana do kernel e `31 ns` para o custo mediano de duas leituras consecutivas do relógio.

Os resultados são registrados em:

```text
config/experiment.conf
config/calibration.json
```

`experiment.conf` contém a configuração congelada utilizada nos experimentos, enquanto `calibration.json` preserva os dados que fundamentam sua seleção.

A calibração não integra a região cronometrada dos experimentos.

## 7. Comparação entre as versões

A comparação utiliza os tamanhos:

```text
T = 32, 64, 128, 256, 512, 1024, 2048, 4096
```

com `N=8192` constante.

As versões avaliadas são:

* `base` — implementação base;
* `optimized` — implementação otimizada.

A versão otimizada preserva o mesmo cálculo, a precisão `double`, a organização principal dos dados e a execução em um único fluxo. A otimização concentra-se no laço interno e utiliza:

* SIMD AVX de 256 bits;
* quatro acumuladores vetoriais independentes;
* desenrolamento em blocos de 16 elementos;
* qualificadores `restrict`;
* tratamento vetorial e escalar dos elementos remanescentes.

A comparação completa é executada com:

```bash
./scripts/run_comparison_experiments.sh
python3 scripts/analyze_comparison.py
python3 scripts/plot_comparison.py
```

A ordem das combinações entre implementação e tamanho é intercalada e pseudoaleatória de forma determinística. As duas versões utilizam a mesma configuração experimental, e todas as observações são preservadas.

## 8. Dados produzidos

A execução comparativa produz:

```text
data/part2/raw_measurements_comparison.csv
data/part2/measurements_comparison_with_metrics.csv
data/part2/summary_comparison.csv
data/part2/comparison_table.csv
```

`raw_measurements_comparison.csv` preserva as aferições individuais.

`measurements_comparison_with_metrics.csv` contém as aferições acrescidas das métricas derivadas.

`summary_comparison.csv` apresenta as estatísticas agregadas por implementação e tamanho.

`comparison_table.csv` reúne os principais valores utilizados na comparação apresentada no relatório.

## 9. Métricas

Para um tempo de execução `t`, a taxa de vetores é:

$$
P_v=\frac{N}{t}.
$$

A taxa de elementos é:

$$
P_e=\frac{NT}{t}.
$$

O custo temporal por elemento é:

$$
C_e=\frac{t\times10^9}{NT}
$$

em ns/elemento.

O fator de aceleração da versão otimizada é calculado por:

$$
S(T)=
\frac{t_{\mathrm{base}}(T)}
     {t_{\mathrm{otimizada}}(T)}.
$$

A mediana é utilizada como medida central, mantendo-se as aferições individuais para análise da variabilidade.

## 10. Gráficos

Os gráficos comparativos são armazenados em:

```text
results/part2/
```

São produzidos:

* `execution_time_comparison.png` — tempo mediano de execução;
* `vectors_per_second_comparison.png` — vazão de vetores;
* `elements_per_second_comparison.png` — vazão de elementos;
* `ns_per_element_comparison.png` — tempo por elemento;
* `execution_time_variability_comparison.png` — distribuição dos tempos;
* `speedup.png` — fator de aceleração da versão otimizada.

## 11. Plataforma experimental

Os resultados apresentados no relatório foram obtidos em:

```text
Lenovo ThinkPad T530
Intel Core i5-3320M @ 2.60 GHz
Arquitetura x86-64
Família 6, modelo 58
AVX
SSE4.1
SSE4.2
GCC 10.5.0
Ubuntu 24.04.4 LTS
```

O benchmark foi compilado com:

```text
/usr/bin/g++
```

e executado em um único CPU lógico, conforme a configuração determinada durante a calibração.

As informações detalhadas do sistema e dos parâmetros experimentais são preservadas em `system/`.

## 12. Reprodutibilidade

Os principais artefatos para reprodução e auditoria dos resultados incluem:

```text
config/experiment.conf
config/calibration.json
data/part2/raw_measurements_comparison.csv
data/part2/measurements_comparison_with_metrics.csv
data/part2/summary_comparison.csv
data/part2/comparison_table.csv
system/system_info.txt
system/experiment_parameters.txt
```

Esses arquivos preservam a configuração experimental, a calibração, as aferições individuais, as estatísticas derivadas e a caracterização da plataforma.

## 13. Sequência completa

Após clonar o repositório:

```bash
python3 -m pip install -r requirements.txt

export CXX=/usr/bin/g++

make clean
make release
make test
make vectorization

python3 scripts/calibrate_n.py

./scripts/run_comparison_experiments.sh
python3 scripts/analyze_comparison.py
python3 scripts/plot_comparison.py
```

Após a existência de uma configuração experimental válida, a Parte 2 também pode ser executada por:

```bash
make part2
```

A interface final pode ser verificada separadamente com:

```bash
./build/vector_distance 1000
```

cuja única saída padrão segue a forma:

```text
xavier, 1000, <tempo_ms>
```

## 14. Estrutura principal do repositório

```text
.
├── Makefile
├── README.md
├── PART2_OPTIMIZATION.md
├── LATE_SPECIFICATION.md
├── requirements.txt
├── src/
├── config/
│   ├── experiment.conf
│   └── calibration.json
├── scripts/
│   ├── calibrate_n.py
│   ├── run_comparison_experiments.sh
│   ├── analyze_comparison.py
│   └── plot_comparison.py
├── data/
│   └── part2/
│       ├── raw_measurements_comparison.csv
│       ├── measurements_comparison_with_metrics.csv
│       ├── summary_comparison.csv
│       └── comparison_table.csv
├── results/
│   └── part2/
├── system/
└── PAD_2026___T1___Relatorio_2.pdf
```

## 15. Relatório

O relatório final está disponível em:

```text
PAD_2026___T1___Relatorio_2.pdf
```

O documento apresenta a implementação base, a otimização desenvolvida, a calibração da configuração experimental, a metodologia de aferição, a análise da variabilidade e a comparação quantitativa entre as duas versões.

## Autor

**Xavier Paulino Sebastião**

Processamento de Alto Desempenho — 2026
