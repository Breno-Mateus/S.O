#!/bin/bash

# Compila o programa com a flag de otimização exigida
make

A=100028277
B=8000000000
OUT="resultados.csv"

# Remove arquivo de testes anteriores se existir
rm -f $OUT

# Laço automático dos 16 testes
for W in 1 2 4 8; do
    for modo in processo thread; do
        for particao in bloco ciclico; do
            echo "Rodando: W=$W | Modo=$modo | Partição=$particao"
            ./varredor $A $B $W $modo $particao $OUT
        done
    done
done

echo "Testes concluídos! Confira o arquivo $OUT."