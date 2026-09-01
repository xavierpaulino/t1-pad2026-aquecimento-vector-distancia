CXX ?= g++
STD := -std=c++20
WARN := -Wall -Wextra -Wpedantic -Wconversion -Wshadow
BASE := $(STD) $(WARN) -DNDEBUG
RELEASE_FLAGS ?= -O3 -march=native
PORTABLE_FLAGS ?= -O3
TARGET := build/vector_distance
SRC := src/vector_distance.cpp

.PHONY: all release portable vectorization test clean part2

all: release

release: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p build
	$(CXX) $(BASE) $(RELEASE_FLAGS) $(SRC) -o $(TARGET)

portable:
	@mkdir -p build
	$(CXX) $(BASE) $(PORTABLE_FLAGS) $(SRC) -o build/vector_distance_portable

vectorization:
	@mkdir -p build
	$(CXX) $(BASE) $(RELEASE_FLAGS) -fopt-info-vec-all=build/vectorization.txt $(SRC) -o build/vector_distance_vectorization
	@echo "Vectorization report: build/vectorization.txt"

test: $(TARGET)
	./$(TARGET) --self-test

part2: release test
	./scripts/run_comparison_experiments.sh
	python3 scripts/analyze_comparison.py
	python3 scripts/plot_comparison.py

clean:
	rm -f build/vector_distance build/vector_distance_portable build/vector_distance_vectorization build/vectorization.txt
