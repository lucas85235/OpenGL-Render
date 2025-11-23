#!/bin/bash

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

clear

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Model Loader - OpenGL Compiler${NC}"
echo -e "${BLUE}========================================${NC}\n"

# Verificar dependências
echo -e "${YELLOW}Verificando dependências...${NC}"

check_package() {
    if pkg-config --exists $1 2>/dev/null; then
        echo -e "  ${GREEN}✓${NC} $1"
        return 0
    else
        echo -e "  ${RED}✗${NC} $1 ${RED}(não encontrado)${NC}"
        return 1
    fi
}

missing_deps=0

check_package glew || missing_deps=1
check_package glfw3 || missing_deps=1
check_package assimp || missing_deps=1
check_package glm || missing_deps=1

# Verificar stb_image.h
if [ -f "src/renderer/stb_image.h" ]; then
    echo -e "  ${GREEN}✓${NC} stb_image.h"
else
    echo -e "  ${RED}✗${NC} stb_image.h ${RED}(não encontrado)${NC}"
    echo -e "\n${YELLOW}Baixando stb_image.h...${NC}"
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    if [ $? -eq 0 ]; then
        echo -e "  ${GREEN}✓${NC} stb_image.h baixado com sucesso!"
    else
        echo -e "  ${RED}✗${NC} Falha ao baixar stb_image.h"
        missing_deps=1
    fi
fi

if [ $missing_deps -eq 1 ]; then
    echo -e "\n${RED}Erro: Dependências faltando!${NC}"
    echo -e "${YELLOW}Instale com:${NC}"
    echo -e "  Ubuntu/Debian: ${GREEN}sudo apt-get install libglew-dev libglfw3-dev libassimp-dev libglm-dev${NC}"
    echo -e "  Fedora:        ${GREEN}sudo dnf install glew-devel glfw-devel assimp-devel glm-devel${NC}"
    echo -e "  Arch:          ${GREEN}sudo pacman -S glew glfw-x11 assimp glm${NC}"
    exit 1
fi

echo -e "\n${YELLOW}Compilando...${NC}"

# Flags de compilação
SOURCES="main.cpp"
OUTPUT="model_viewer"
CXX_FLAGS="-std=c++17 -O2 -w"
INCLUDES=""
LIBS="-lGL -lGLEW -lglfw -lassimp -lpthread -ldl"

# Comando de compilação
g++ $SOURCES -o $OUTPUT $CXX_FLAGS $INCLUDES $LIBS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Compilação bem-sucedida!${NC}\n"
    
    # Verificar se existe pasta models/
    if [ ! -d "models" ]; then
        echo -e "${YELLOW}Aviso: Pasta 'models/' não encontrada!${NC}"
        echo -e "${YELLOW}Criando pasta models/...${NC}"
        mkdir -p models
        echo -e "${GREEN}✓ Pasta criada!${NC}\n"
        echo -e "${BLUE}Coloque seus modelos 3D em:${NC} models/"
        echo -e "${BLUE}Exemplo:${NC} models/backpack/backpack.obj\n"
    fi
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}Executando aplicação...${NC}"
    echo -e "${BLUE}========================================${NC}\n"
    
    ./$OUTPUT
    
else
    echo -e "${RED}✗ Erro na compilação!${NC}"
    exit 1
fi
