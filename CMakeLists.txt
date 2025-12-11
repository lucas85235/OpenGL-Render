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

# Vulkan
find_package(Vulkan REQUIRED)

# GLEW
find_package(GLEW REQUIRED)

# GLFW3
find_package(glfw3 3.3 REQUIRED)

# GLM
find_package(glm REQUIRED)

# ASSIMP
find_package(assimp REQUIRED)

# SPIRV-Cross (Optional - for cross-compiling SPIR-V to GLSL)
# Install with: sudo apt install libspirv-cross-c-shared-dev
# Or build from source: https://github.com/KhronosGroup/SPIRV-Cross
option(USE_SPIRV_CROSS "Enable SPIRV-Cross for shader cross-compilation" OFF)
if(USE_SPIRV_CROSS)
    find_package(spirv_cross_core QUIET)
    find_package(spirv_cross_glsl QUIET)
    if(spirv_cross_core_FOUND AND spirv_cross_glsl_FOUND)
        message(STATUS "SPIRV-Cross found - shader cross-compilation enabled")
        add_compile_definitions(HAS_SPIRV_CROSS=1)
    else()
        # Try pkg-config fallback
        pkg_check_modules(SPIRV_CROSS spirv-cross-c-shared)
        if(SPIRV_CROSS_FOUND)
            message(STATUS "SPIRV-Cross found via pkg-config")
            add_compile_definitions(HAS_SPIRV_CROSS=1)
        else()
            message(WARNING "SPIRV-Cross not found. Install libspirv-cross-c-shared-dev or disable USE_SPIRV_CROSS")
            set(USE_SPIRV_CROSS OFF)
        endif()
    endif()
endif()

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
# Vulkan Shader Compilation (GLSL -> SPIR-V)
# ==========================================
find_program(GLSLC glslc HINTS ${Vulkan_GLSLC_EXECUTABLE} ENV VULKAN_SDK PATH_SUFFIXES bin)
find_program(GLSLANG glslangValidator)

if(GLSLC)
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILE_FLAGS "")
    message(STATUS "Found glslc: ${GLSLC}")
elseif(GLSLANG)
    set(SHADER_COMPILER ${GLSLANG})
    set(SHADER_COMPILE_FLAGS "-V")
    message(STATUS "Found glslangValidator: ${GLSLANG}")
endif()

if(SHADER_COMPILER)
    # Vulkan-specific shaders (legacy, for backwards compatibility)
    set(VULKAN_SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/shaders/vulkan")
    set(VULKAN_SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders/vulkan")
    
    # Unified shaders (cross-platform via SPIRV-Cross)
    set(UNIFIED_SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/shaders/unified")
    set(UNIFIED_SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders/unified")
    
    file(MAKE_DIRECTORY ${VULKAN_SHADER_BINARY_DIR})
    file(MAKE_DIRECTORY ${UNIFIED_SHADER_BINARY_DIR})
    
    # Collect all shader sources
    file(GLOB VULKAN_SHADER_SOURCES 
        "${VULKAN_SHADER_SOURCE_DIR}/*.vert"
        "${VULKAN_SHADER_SOURCE_DIR}/*.frag"
        "${VULKAN_SHADER_SOURCE_DIR}/*.comp"
        "${VULKAN_SHADER_SOURCE_DIR}/*.geom"
    )
    
    file(GLOB UNIFIED_SHADER_SOURCES
        "${UNIFIED_SHADER_SOURCE_DIR}/*.vert"
        "${UNIFIED_SHADER_SOURCE_DIR}/*.frag"
        "${UNIFIED_SHADER_SOURCE_DIR}/*.comp"
        "${UNIFIED_SHADER_SOURCE_DIR}/*.geom"
    )
    
    set(SPIRV_OUTPUTS)
    
    # Compile Vulkan shaders
    foreach(SHADER ${VULKAN_SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPIRV_OUTPUT "${VULKAN_SHADER_BINARY_DIR}/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAGS} ${SHADER} -o ${SPIRV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling Vulkan shader: ${SHADER_NAME}"
        )
        list(APPEND SPIRV_OUTPUTS ${SPIRV_OUTPUT})
    endforeach()
    
    # Compile Unified shaders
    foreach(SHADER ${UNIFIED_SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPIRV_OUTPUT "${UNIFIED_SHADER_BINARY_DIR}/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAGS} ${SHADER} -o ${SPIRV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling unified shader: ${SHADER_NAME}"
        )
        list(APPEND SPIRV_OUTPUTS ${SPIRV_OUTPUT})
    endforeach()
    
    add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_OUTPUTS})
else()
    message(FATAL_ERROR "No SPIR-V shader compiler found. Install glslang-tools or Vulkan SDK.")
endif()

# Copiar Shaders para a pasta de build
file(COPY ${CMAKE_SOURCE_DIR}/src/shaders DESTINATION ${CMAKE_BINARY_DIR})

# (Opcional) Copiar Models também se ainda não estiver fazendo
file(COPY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})

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
    Vulkan::Vulkan
    glfw
    assimp
    glm::glm
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# SPIRV-Cross conditional linking
if(USE_SPIRV_CROSS)
    if(spirv_cross_core_FOUND AND spirv_cross_glsl_FOUND)
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE
            spirv-cross-core
            spirv-cross-glsl
        )
    elseif(SPIRV_CROSS_FOUND)
        target_include_directories(${EXECUTABLE_NAME} PRIVATE ${SPIRV_CROSS_INCLUDE_DIRS})
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${SPIRV_CROSS_LIBRARIES})
    endif()
endif()

# ==========================================
# Pós-Build (Criação da pasta models)
# ==========================================
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/models
    COMMENT "Verificando/Criando diretório 'models'..."
)

message(STATUS "Configuração concluída. Pronto para compilar: ${EXECUTABLE_NAME}")

# Define uma macro ROOT_DIR contendo o caminho absoluto do projeto
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}/")

# Garante que usamos C++17 para ter acesso ao std::filesystem
target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_17)