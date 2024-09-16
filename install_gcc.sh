#!/bin/bash

# Instala el compilador GCC
sudo dnf install -y gcc

# Verifica la instalación
gcc --version

sudo dnf install git

make
