cmake_minimum_required(VERSION 3.12)
project(OpenGL_Engine VERSION 1.0.0 LANGUAGES CXX)

# ==========================================
# Configurações do Projeto
# ==========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Gera json para autocomplete em editores (VSCode, etc)

# Configuração de Output
set(EXECUTABLE_NAME "model_viewer")

# Flags de Compilação (Port do seu script: -O2 -w)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O2 -w)
endif()

# ==========================================
# Dependências (Substitui pkg-config manual)
# ==========================================
find_package(PkgConfig REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

# GLEW
find_package(GLEW REQUIRED)

# GLFW3
find_package(glfw3 3.3 REQUIRED)

# GLM
find_package(glm REQUIRED)

# ASSIMP
find_package(assimp REQUIRED)

# Threads & DL (Necessário para C++)
find_package(Threads REQUIRED)

# ==========================================
# Download Automático do stb_image.h
# ==========================================
set(STB_IMAGE_PATH "${CMAKE_SOURCE_DIR}/src/renderer/stb_image.h")
if(NOT EXISTS ${STB_IMAGE_PATH})
    message(STATUS "stb_image.h não encontrado. Baixando...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
        ${STB_IMAGE_PATH}
        SHOW_PROGRESS
    )
endif()

# ==========================================
# Definição do Executável
# ==========================================
# Adiciona todos os arquivos .cpp e .hpp para aparecerem na IDE
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp" "main.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCES})

# ==========================================
# Includes e Links
# ==========================================
# Permite incluir arquivos como "src/core/..." a partir da raiz
target_include_directories(${EXECUTABLE_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
)

# Linkagem das bibliotecas (-lGL -lGLEW -lglfw -lassimp ...)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glfw
    assimp
    glm::glm
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==========================================
# Pós-Build (Criação da pasta models)
# ==========================================
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/models
    COMMENT "Verificando/Criando diretório 'models'..."
)

message(STATUS "Configuração concluída. Pronto para compilar: ${EXECUTABLE_NAME}")