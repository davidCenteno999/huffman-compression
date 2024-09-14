#!/bin/bash

# Instala el compilador GCC
sudo dnf install -y gcc

# Verifica la instalación
gcc --version

sudo dnf install git

git clone https://github.com/davidCenteno999/huffman-compression.git

cd huffman-compression

make
