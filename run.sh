#!/bin/bash
clear
echo "Compilando projeto..."
g++ main.cpp -o framebuffer -lGL -lGLEW -lglfw -lm
if [ $? -eq 0 ]; then
    echo "Compilação bem-sucedida!"
    echo "Executando..."
    ./framebuffer
else
    echo "Erro na compilação!"
fi