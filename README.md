# T1: Aquecimento — Distância Quadrática entre Vetores

Este repositório contém as implementações base e otimizada e a infraestrutura experimental utilizadas para avaliar o desempenho do cálculo da distância quadrática entre um vetor de referência e um conjunto de vetores.

Para um vetor de referência `q` e um conjunto de `N` vetores `x_i`, cada um com `T` elementos reais, é calculada:

$$
D(q,x_i)=\sum_{j=0}^{T-1}(q_j-x_{ij})^2,
\qquad i=0,\ldots,N-1.
$$

A implementação otimizada utiliza SIMD AVX, desenrolamento do laço, quatro acumuladores vetoriais independentes e qualificadores `restrict`, mantendo a execução em um único fluxo.

A região cronometrada contém exclusivamente o cálculo das `N` distâncias e o armazenamento dos resultados. Alocação de memória, inicialização e geração dos dados, execuções de aquecimento, cálculo de checksum, escrita de arquivos, análise estatística e apresentação dos resultados permanecem fora da medição.

---

## 1. Requisitos

Os experimentos foram desenvolvidos para GNU/Linux x86-64.

São necessários:

- GCC com suporte a C++20;
- GNU Make;
- Python 3;
- `taskset`;
- NumPy;
- pandas;
- Matplotlib.

Ferramentas complementares utilizadas na caracterização do ambiente incluem `lscpu` e `perf`.

---

## 2. Clonar o repositório

```bash
git clone https://github.com/xavierpaulino/t1-pad2026-aquecimento-distancia-euclidiana.git
cd t1-pad2026-aquecimento-distancia-euclidiana
```

---

## 3. Preparar o ambiente

As dependências Python podem ser instaladas com:

```bash
python3 -m pip install -r requirements.txt
```

Opcionalmente, pode ser utilizado um ambiente Conda:

```bash
conda create -n t1_vector_distance python=3.12 numpy pandas matplotlib -c conda-forge
conda activate t1_vector_distance
```

O benchmark C++ utiliza o GCC do sistema:

```bash
export CXX=/usr/bin/g++
```

Essa configuração evita que um compilador fornecido pelo ambiente Conda introduza dependências ou requisitos de arquitetura diferentes dos disponíveis na plataforma utilizada nos experimentos.

---

## 4. Compilar e validar

Para limpar compilações anteriores e compilar a versão de desempenho:

```bash
make clean
make release
```

Para executar os testes de corretude:

```bash
make test
```

Uma execução correta produz:

```text
SELF_TEST_OK
```

A compilação de desempenho utiliza C++20 com:

```text
-O3 -march=native
```

Não é utilizado `-ffast-math`.

O relatório de vetorização produzido pelo GCC pode ser gerado com:

```bash
make vectorization
```

e é armazenado em:

```text
build/vectorization.txt
```

---

## 5. Interface de execução

O tamanho do vetor `T` é informado como **primeiro argumento posicional** do executável:

```bash
./build/vector_distance T
```

Por exemplo:

```bash
./build/vector_distance 1000
```

A saída padrão contém **exclusivamente uma linha**, no formato:

```text
xavier, 1000, 313
```

Os campos correspondem a:

```text
identificação, tamanho_do_vetor, tempo_em_milissegundos
```

Portanto, uma saída como:

```text
xavier, 1000, 313
```

indica que o programa identificado por `xavier`, executado com vetor de tamanho `T=1000`, apresentou tempo de execução de `313` milissegundos.

O valor do tempo pode conter casas decimais e permanece expresso em milissegundos.

Caso sejam necessários parâmetros adicionais, estes são informados **depois do tamanho do vetor**, preservando `T` como primeiro argumento.

As execuções de aquecimento não integram o tempo informado.

Alocação, inicialização dos dados, aquecimento, cálculo de checksum e impressão também permanecem fora da região cronometrada.

## 6. Calibração experimental

Antes da avaliação comparativa, a configuração experimental é determinada por uma etapa de calibração independente das medições utilizadas nos resultados finais.

A calibração pode ser executada com:

```bash
python3 scripts/calibrate_n.py
```

O procedimento considera o maior tamanho avaliado:

```text
T = 4096
```

e seleciona o maior valor admissível de `N` segundo o critério de memória definido para o experimento.

A calibração também seleciona deterministicamente um CPU lógico permitido, considerando a topologia física do processador.

A configuração selecionada na nesta execução:

```text
N = 16384
CPU lógico = 2
T máximo = 4096
```
A adequação da temporização foi avaliada utilizando `CLOCK_MONOTONIC_RAW`.

Os principais resultados da calibração foram:

```text
Mediana do kernel para T=32:       0,441797 ms
Resolução nominal do relógio:      1 ns
Mediana entre leituras do relógio: 28 ns
Razão temporizador/kernel:          0,006338%
Limite de aceitação:                1,000000%
```

Como a razão observada ficou muito abaixo do limite de 1%, a configuração foi aceita.

Os resultados da calibração e a configuração congelada são preservados em:

```text
config/calibration.json
config/experiment.conf
```

A calibração não integra as medições utilizadas na comparação final.

---

## 7. Avaliação comparativa

A avaliação compara as implementações base e otimizada utilizando:

```text
N = 16384
CPU lógico = 2
seed = 42
```

para todos os tamanhos:

```text
T = 32, 64, 128, 256, 512, 1024, 2048, 4096
```

As implementações avaliadas são:

- `base` — implementação de referência;
- `optimized` — implementação otimizada.

A versão otimizada preserva:

- o mesmo cálculo;
- precisão `double`;
- o mesmo layout principal dos dados;
- execução em um único fluxo.

A otimização concentra-se no laço interno e utiliza:

- SIMD AVX de 256 bits;
- quatro acumuladores vetoriais independentes;
- desenrolamento do laço em blocos de 16 elementos;
- qualificadores `restrict`;
- tratamento vetorial e escalar dos elementos remanescentes.

---

## 8. Procedimento de medição

Cada combinação entre implementação e tamanho do vetor foi medida:

```text
30 vezes
```

As medições foram distribuídas em:

```text
5 blocos × 6 repetições
```

Em cada bloco, cada combinação foi precedida por:

```text
2 execuções de aquecimento
```

A configuração experimental é:

```text
N = 16384
SEED = 42
REPETIÇÕES = 30
BLOCOS = 5
REPETIÇÕES POR BLOCO = 6
AQUECIMENTOS = 2
CPU = 2
```

A ordem das combinações `(implementação, T)` é pseudoaleatória e determinística. Esse procedimento reduz a associação sistemática entre determinada implementação ou tamanho de entrada e um momento específico da execução.

Todas as observações são preservadas. Valores discrepantes não são removidos automaticamente.

A região cronometrada contém exclusivamente o cálculo e o armazenamento das `N` distâncias.

Não fazem parte da medição:

- alocação de memória;
- geração dos dados;
- inicialização dos vetores;
- execuções de aquecimento;
- cálculo de checksum;
- escrita de arquivos;
- análise estatística;
- geração dos gráficos;
- impressão dos resultados.

A temporização utiliza:

```text
CLOCK_MONOTONIC_RAW
```

---

## 9. Reproduzir a comparação

A configuração correspondente aos resultados finais está registrada em:

```text
config/experiment.conf
```

A comparação entre as versões pode ser reproduzida com:

```bash
./scripts/run_comparison_experiments.sh
python3 scripts/analyze_comparison.py
python3 scripts/plot_comparison.py
```

## 10. Dados produzidos

A execução comparativa produz:

```text
data/part2/raw_measurements_comparison.csv
data/part2/measurements_comparison_with_metrics.csv
data/part2/summary_comparison.csv
data/part2/comparison_table.csv
```

### `raw_measurements_comparison.csv`

Contém as medições individuais realizadas para cada combinação entre implementação e tamanho.

### `measurements_comparison_with_metrics.csv`

Contém as medições individuais acrescidas das métricas derivadas.

### `summary_comparison.csv`

Apresenta as estatísticas agregadas por implementação e tamanho.

### `comparison_table.csv`

Reúne os principais resultados utilizados na comparação apresentada no relatório.

---

## 11. Métricas de desempenho

Além do tempo de execução, são analisadas a vazão de vetores, a vazão de elementos, o custo temporal por elemento, a variabilidade e o fator de aceleração.

### Vazão de vetores

Para um tempo de execução `t`:
$$
\[
P_v=\frac{N}{t}.
\]
$$
### Vazão de elementos
$$
\[
P_e=\frac{NT}{t}.
\]
$$
### Custo temporal por elemento
$$
\[
C_e=\frac{t\times10^9}{NT}
\]
$$
em ns/elemento.

### Fator de aceleração

O fator de aceleração da versão otimizada em relação à versão base é:
$$
\[
S(T)=
\frac{t_{\mathrm{base}}(T)}
     {t_{\mathrm{otimizada}}(T)}.
\]
$$
Valores de $S(T) > 1$ indicam menor tempo de execução da versão otimizada.

A redução percentual do tempo é:
$$
\[
R(T)=100
\left(
1-\frac{t_{\mathrm{otimizada}}(T)}
        {t_{\mathrm{base}}(T)}
\right).
\]
$$
A mediana é utilizada como medida central, mantendo-se as observações individuais para análise da variabilidade.

---

## 12. Resultados

A versão otimizada apresentou menor tempo mediano e menor custo por elemento em **todos os tamanhos avaliados**.

| T | Base (ms) | Otimizada (ms) | Base (10⁹ elem./s) | Otimizada (10⁹ elem./s) | Base (ns/elem.) | Otimizada (ns/elem.) | Aceleração | Redução |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 32 | 0,478 | 0,392 | 1,098 | 1,336 | 0,911 | 0,749 | 1,22× | 17,8% |
| 64 | 1,047 | 0,805 | 1,001 | 1,302 | 0,999 | 0,768 | 1,30× | 23,1% |
| 128 | 2,160 | 1,503 | 0,971 | 1,395 | 1,030 | 0,717 | 1,44× | 30,4% |
| 256 | 4,708 | 2,907 | 0,891 | 1,443 | 1,123 | 0,693 | 1,62× | 38,3% |
| 512 | 9,373 | 5,817 | 0,895 | 1,442 | 1,117 | 0,693 | 1,61× | 37,9% |
| 1024 | 19,227 | 11,092 | 0,873 | 1,513 | 1,146 | 0,661 | **1,73×** | **42,3%** |
| 2048 | 40,187 | 27,750 | 0,835 | 1,209 | 1,198 | 0,827 | 1,45× | 30,9% |
| 4096 | 77,485 | 51,536 | 0,866 | 1,302 | 1,155 | 0,768 | 1,50× | 33,5% |

O fator de aceleração permaneceu acima de 1 em toda a faixa avaliada, variando aproximadamente entre:

```text
1,22× e 1,73×
```

O maior ganho foi observado em:

```text
T = 1024
```

com fator de aceleração de aproximadamente:

```text
1,73×
```

e redução de aproximadamente:

```text
42,3%
```

no tempo mediano.

Em `T=2048`, o fator de aceleração diminuiu para aproximadamente `1,45×`, acompanhado por redução da vazão de elementos e aumento do custo por elemento da versão otimizada.

Em `T=4096`, houve recuperação parcial, com fator de aceleração de aproximadamente `1,50×`.

Os resultados mostram que a otimização é vantajosa em todos os tamanhos avaliados, embora a magnitude do ganho não cresça monotonicamente com `T`.

As medições disponíveis permitem quantificar essas diferenças, mas não são suficientes, isoladamente, para atribuir as variações observadas a mecanismos microarquiteturais específicos.

---

## 13. Gráficos

Os gráficos comparativos são armazenados em:

```text
results/part2/
```

São produzidos:

- `execution_time_comparison.png` — tempo mediano de execução;
- `vectors_per_second_comparison.png` — vazão mediana de vetores;
- `elements_per_second_comparison.png` — vazão mediana de elementos;
- `ns_per_element_comparison.png` — custo mediano por elemento;
- `execution_time_variability_comparison.png` — distribuição dos tempos de execução;
- `speedup.png` — fator de aceleração da versão otimizada.

Os gráficos utilizados no relatório apresentam títulos, eixos, legendas e unidades em português.

---

## 14. Plataforma experimental

Os resultados apresentados no relatório foram obtidos em:

```text
Lenovo ThinkPad T530
Intel Core i5-3320M @ 2.60 GHz
Arquitetura x86-64
2 núcleos físicos
4 CPUs lógicos
Família 6, modelo 58
AVX
SSE4.1
SSE4.2
GCC 10.5.0
Ubuntu 24.04.4 LTS
Kernel Linux 7.0.0-30-generic
```

O benchmark foi compilado com:

```text
/usr/bin/g++
```

utilizando:

```text
-O3 -march=native
```

A execução experimental foi fixada no CPU lógico:

```text
2
```

O CPU 2 pertence ao núcleo físico cujo conjunto de CPUs lógicos é:

```text
[2, 3]
```

Durante a caracterização do sistema, o governador de frequência registrado foi:

```text
schedutil
```

e o turbo estava habilitado.

Informações mais detalhadas da plataforma e dos parâmetros experimentais são preservadas no diretório:

```text
system/
```

---

## 15. Reprodutibilidade

Os principais artefatos utilizados para reprodução e auditoria dos resultados são:

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

Esses arquivos preservam a configuração experimental, os dados de calibração, as medições individuais, as métricas derivadas, os parâmetros da comparação e a caracterização da plataforma.

---

## 16. Sequência completa de execução

Após clonar o repositório:

```bash
git clone https://github.com/xavierpaulino/t1-pad2026-aquecimento-distancia-euclidiana.git
cd t1-pad2026-aquecimento-distancia-euclidiana
```

Instalar as dependências:

```bash
python3 -m pip install -r requirements.txt
```

Selecionar o compilador:

```bash
export CXX=/usr/bin/g++
```

Compilar e validar:

```bash
make clean
make release
make test
```

Executar a calibração:

```bash
python3 scripts/calibrate_n.py
```

Executar a comparação:

```bash
./scripts/run_comparison_experiments.sh
```

Analisar os resultados:

```bash
python3 scripts/analyze_comparison.py
```

Gerar os gráficos:

```bash
python3 scripts/plot_comparison.py
```

Opcionalmente, gerar o relatório de vetorização:

```bash
make vectorization
```

Após a existência de uma configuração experimental válida, a Parte 2 também pode ser executada por:

```bash
make part2
```

---

## 17. Verificação da interface de submissão

Para verificar diretamente a interface padronizada:

```bash
./build/vector_distance 1000
```

A saída padrão deve conter exclusivamente uma linha no formato:

```text
xavier, 1000, <tempo_ms>
```

Por exemplo:

```text
xavier, 1000, 15.327418
```

O valor efetivamente obtido depende da execução e da plataforma.

O primeiro argumento corresponde sempre ao tamanho do vetor `T`. Caso sejam utilizados parâmetros adicionais, eles devem aparecer depois desse argumento.

---

## 18. Estrutura principal do repositório

```text
.
├── Makefile
├── README.md
├── PART2_OPTIMIZATION.md
├── LATE_SPECIFICATION.md
├── requirements.txt
│
├── src/
│   └── vector_distance.cpp
│
├── config/
│   ├── experiment.conf
│   └── calibration.json
│
├── scripts/
│   ├── calibrate_n.py
│   ├── run_comparison_experiments.sh
│   ├── analyze_comparison.py
│   └── plot_comparison.py
│
├── data/
│   └── part2/
│       ├── raw_measurements_comparison.csv
│       ├── measurements_comparison_with_metrics.csv
│       ├── summary_comparison.csv
│       └── comparison_table.csv
│
├── results/
│   └── part2/
│       ├── execution_time_comparison.png
│       ├── vectors_per_second_comparison.png
│       ├── elements_per_second_comparison.png
│       ├── ns_per_element_comparison.png
│       ├── execution_time_variability_comparison.png
│       └── speedup.png
│
├── system/
│   ├── system_info.txt
│   ├── experiment_parameters.txt
│   └── part2_experiment_parameters.txt
│
└── PAD_2026___T1___Relatorio_2.pdf
```

---

## 19. Relatório

O relatório final está disponível em:

```text
PAD_2026___T1___Relatorio_2.pdf
```

O documento apresenta:

- a definição do problema;
- a implementação base;
- a implementação otimizada;
- a calibração da configuração experimental;
- o procedimento de medição;
- as métricas de desempenho;
- a análise da variabilidade;
- a comparação quantitativa entre as implementações;
- a discussão dos resultados.

---

## Autor

**Xavier Paulino Sebastião**

Processamento de Alto Desempenho — 2026
