# Makefile - Parcial 1 SO
CXX := g++
CC  := gcc
CXXFLAGS := -O3 -pipe -march=native -flto -std=c++20 -Wall -Wextra -Wshadow -Wconversion -DNDEBUG
CFLAGS   := -O3 -pipe -march=native -flto -std=c17   -Wall -Wextra -Wshadow -Wconversion -DNDEBUG
LDFLAGS  := -flto

BIN := bin
SRC_CPP := src/cpp
SRC_C   := src/c

all: build

$(BIN)/so_parcial_cpp_values: $(SRC_CPP)/main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN)/so_parcial_cpp_pointers: $(SRC_CPP)/main.cpp
	$(CXX) $(CXXFLAGS) -DUSE_POINTERS=1 $< -o $@ $(LDFLAGS)

$(BIN)/so_parcial_c_values: $(SRC_C)/main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BIN)/so_parcial_c_pointers: $(SRC_C)/main.c
	$(CC) $(CFLAGS) -DUSE_POINTERS=1 $< -o $@ $(LDFLAGS)

build: $(BIN)/so_parcial_cpp_values $(BIN)/so_parcial_cpp_pointers $(BIN)/so_parcial_c_values $(BIN)/so_parcial_c_pointers

clean:
	rm -f $(BIN)/*

.PHONY: all build clean
