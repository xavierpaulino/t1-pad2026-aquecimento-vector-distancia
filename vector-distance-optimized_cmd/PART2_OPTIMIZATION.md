# Parte 2 — primeira otimização

Esta versão preserva integralmente o kernel da Parte 1 como `baseline_v3` e adiciona `optimized_v4` para comparação sob as mesmas condições experimentais.

## Escopo da otimização

A primeira otimização é deliberadamente **single-thread**. Isso mantém o mesmo orçamento de recursos da Parte 1 e permite atribuir diferenças de desempenho à implementação do kernel, sem confundir otimização de código com uso de mais cores.

O kernel otimizado aplica:

- ponteiros `__restrict__`, removendo ambiguidade de aliasing no kernel otimizado;
- SIMD AVX quando disponível em compilação `-march=native`;
- desenrolamento explícito do laço interno em blocos de 16 elementos;
- quatro acumuladores vetoriais independentes, reduzindo a cadeia de dependência da redução;
- tratamento vetorial e escalar do restante, preservando suporte a qualquer `T`;
- as mesmas flags principais da Parte 1 (`-O3 -march=native`), sem `-ffast-math`.

A ordem da soma muda no kernel SIMD. Por isso os resultados de ponto flutuante podem não ser bit a bit idênticos aos da versão escalar. `make test` compara os dois kernels em vários tamanhos, inclusive tamanhos que exercitam os caminhos de resto, usando tolerância numérica estrita.

## Comparação experimental

Para evitar uma comparação temporalmente enviesada entre uma campanha antiga e uma nova, a Parte 2 executa **baseline v3 e optimized v4 novamente na mesma campanha**, intercalando deterministicamente as combinações `(variante, T)` dentro de cada bloco.

A configuração congelada da Parte 1 (`N`, CPU, seed, repetições e warm-up) é reutilizada. O script de comparação aborta se `config/experiment.conf` não existir; ele não recalibra automaticamente.

### Execução

```bash
export CXX=/usr/bin/g++
make clean
make release
make test
make vectorization
./scripts/run_comparison_experiments.sh
python3 scripts/analyze_comparison.py
python3 scripts/plot_comparison.py
```

### Saídas

- `data/part2/raw_measurements_comparison.csv`
- `data/part2/measurements_comparison_with_metrics.csv`
- `data/part2/summary_comparison.csv`
- `data/part2/comparison_table.csv`
- `results/part2/execution_time_comparison.{png,pdf}`
- `results/part2/vectors_per_second_comparison.{png,pdf}`
- `results/part2/elements_per_second_comparison.{png,pdf}`
- `results/part2/ns_per_element_comparison.{png,pdf}`
- `results/part2/execution_time_variability_comparison.{png,pdf}`
- `results/part2/speedup.{png,pdf}`

`comparison_table.csv` inclui os tempos medianos das duas versões, speedup, redução percentual de tempo, throughput, custo por elemento, CV e speedup calculado por blocos pareados.
