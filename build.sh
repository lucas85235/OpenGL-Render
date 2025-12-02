#!/bin/bash

# Configurações
BUILD_DIR="build"
BUILD_TYPE="Release" # Debug ou Release

# Cores
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

clear

echo -e "${BLUE}=== Iniciando Build System (CMake + Ninja) ===${NC}"

# 1. Gera os arquivos de build se a pasta não existir ou se for forçado
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${BLUE}Configurando projeto...${NC}"
    cmake -B $BUILD_DIR -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE
else
    # Opcional: Reconfigura apenas se necessário
    echo -e "${BLUE}Verificando alterações de configuração...${NC}"
    cmake -B $BUILD_DIR -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE
fi

# 2. Compila usando Ninja
echo -e "\n${BLUE}Compilando com Ninja...${NC}"
ninja -C $BUILD_DIR

# 3. Executa se a compilação for bem sucedida
if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}✓ Build Sucesso! Executando...${NC}\n"
    # Copia o executável para a raiz ou executa direto de lá
    cp $BUILD_DIR/model_viewer .
    ./model_viewer
else
    echo -e "\n${RED}✗ Falha na compilação.${NC}"
    exit 1
fi