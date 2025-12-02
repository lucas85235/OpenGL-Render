#define FS FileSystem

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <string>
#include <cstdlib>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

class FileSystem {
public:
    // Retorna o caminho absoluto correto para um arquivo
    static std::string GetPath(const std::string& path) {
        // 1. Tenta relativo ao executável (Pasta Build/Release)
        // Se o CMake copiou a pasta "shaders" para "build/shaders", isso funciona direto.
        if (fs::exists(path)) {
            return path;
        }

        #ifdef ROOT_DIR
            // O ROOT_DIR é a raiz do seu projeto (definida no CMakeLists.txt)
            std::string root = std::string(ROOT_DIR);

            // 2. Tenta direto na raiz do projeto
            // Útil para "models/car.obj" que fica em "raiz/models/"
            std::string rootPath = root + path;
            if (fs::exists(rootPath)) {
                return rootPath;
            }

            // 3. Tenta dentro de 'src/' (Correção para o seu caso)
            // Útil para "shaders/pbr.vert" que na verdade está em "raiz/src/shaders/"
            std::string srcPath = root + "src/" + path;
            if (fs::exists(srcPath)) {
                return srcPath;
            }
        #endif

        // Se falhar, retorna o original e deixa o erro acontecer no log do Shader/Texture
        // std::cerr << "[FS] Arquivo não encontrado: " << path << std::endl;
        return path;
    }

    static std::string GetRoot() {
        #ifdef ROOT_DIR
            return ROOT_DIR;
        #else
            return fs::current_path().string();
        #endif
    }
};

#endif // FILESYSTEM_HPP