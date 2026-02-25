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
set(STB_IMAGE_PATH "${CMAKE_SOURCE_DIR}/src/vendor/stb_image.h")
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
    # ==========================================
    # Unified Shader Format (.shader -> SPIR-V)
    # Uses Python preprocessor for multi-stage shaders
    # ==========================================
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
        set(UNIFIED_SHADER_SCRIPT "${CMAKE_SOURCE_DIR}/tools/compile_shaders.py")
        set(UNIFIED_SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
        set(UNIFIED_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
        
        file(MAKE_DIRECTORY ${UNIFIED_SHADER_OUTPUT_DIR})
        file(GLOB SHADER_FILES "${UNIFIED_SHADER_INPUT_DIR}/*.shader")
        
        if(SHADER_FILES)
            set(UNIFIED_SPIRV_OUTPUTS)
            foreach(SHADER ${SHADER_FILES})
                get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
                list(APPEND UNIFIED_SPIRV_OUTPUTS 
                    "${UNIFIED_SHADER_OUTPUT_DIR}/${SHADER_NAME}.vert.spv"
                    "${UNIFIED_SHADER_OUTPUT_DIR}/${SHADER_NAME}.frag.spv"
                )
            endforeach()
            
            add_custom_command(
                OUTPUT ${UNIFIED_SPIRV_OUTPUTS}
                COMMAND ${Python3_EXECUTABLE} ${UNIFIED_SHADER_SCRIPT}
                    ${UNIFIED_SHADER_INPUT_DIR} ${UNIFIED_SHADER_OUTPUT_DIR}
                    --api vulkan --version 450
                DEPENDS ${SHADER_FILES} ${UNIFIED_SHADER_SCRIPT}
                COMMENT "Compiling unified .shader files to SPIR-V"
            )
            
            add_custom_target(compile_shaders ALL DEPENDS ${UNIFIED_SPIRV_OUTPUTS})
            message(STATUS "Found ${CMAKE_MATCH_COUNT} unified .shader files")
        else()
            message(WARNING "No .shader files found in ${UNIFIED_SHADER_INPUT_DIR}")
        endif()
    else()
        message(WARNING "Python3 not found - unified .shader compilation disabled")
    endif()
else()
    message(FATAL_ERROR "No SPIR-V shader compiler found. Install glslang-tools or Vulkan SDK.")
endif()

# Copiar Shaders para a pasta de build
file(COPY ${CMAKE_SOURCE_DIR}/src/shaders DESTINATION ${CMAKE_BINARY_DIR})

# (Opcional) Copiar Models também se ainda não estiver fazendo
file(COPY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})

# ==========================================
# Dear ImGui (docking branch)
# ==========================================
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/src/vendor/imgui")
file(GLOB IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")

# ==========================================
# Definição do Executável
# ==========================================
# Adiciona todos os arquivos .cpp e .hpp para aparecerem na IDE
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp" "main.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${IMGUI_SOURCES})

# ==========================================
# Includes e Links
# ==========================================
# Permite incluir arquivos como "src/core/..." a partir da raiz
target_include_directories(${EXECUTABLE_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${IMGUI_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
)

# ImGui uses GLEW as OpenGL loader
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)

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

# Add dependency on unified shader compilation if target exists
if(TARGET compile_unified_shaders)
    add_dependencies(${EXECUTABLE_NAME} compile_unified_shaders)
endif()