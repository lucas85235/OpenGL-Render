#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <GL/glew.h>
#include <string>
#include <iostream>
#include <map>
#include <memory>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

enum class TextureType {
    DIFFUSE,
    SPECULAR,
    NORMAL,
    HEIGHT,
    AMBIENT,
    EMISSION,
    METALLIC,
    ROUGHNESS,
    AO, // Ambient Occlusion
    UNKNOWN
};

enum class TextureWrap {
    REPEAT = GL_REPEAT,
    MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
    CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
    CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER
};

enum class TextureFilter {
    NEAREST = GL_NEAREST,
    LINEAR = GL_LINEAR,
    NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
};

struct TextureParams {
    TextureWrap wrapS = TextureWrap::REPEAT;
    TextureWrap wrapT = TextureWrap::REPEAT;
    TextureFilter minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
    TextureFilter magFilter = TextureFilter::LINEAR;
    bool generateMipmap = true;
    bool flipVertically = true;
};

class Texture {
private:
    unsigned int id;
    std::string path;
    TextureType type;
    int width;
    int height;
    int channels;
    bool loaded;

public:
    Texture() : id(0), width(0), height(0), channels(0), loaded(false) {}

    ~Texture() {
        if (loaded) {
            glDeleteTextures(1, &id);
        }
    }

    bool LoadFromFile(const std::string& filepath, TextureType texType, 
                      const TextureParams& params = TextureParams()) {
        path = filepath;
        type = texType;

        stbi_set_flip_vertically_on_load(params.flipVertically);

        unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
        
        if (!data) {
            std::cerr << "Falha ao carregar textura: " << filepath << std::endl;
            std::cerr << "Erro: " << stbi_failure_reason() << std::endl;
            return false;
        }

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // Determinar formato
        GLenum format = GL_RGB;
        GLenum internalFormat = GL_RGB;
        
        if (channels == 1) {
            format = GL_RED;
            internalFormat = GL_RED;
        } else if (channels == 3) {
            format = GL_RGB;
            internalFormat = GL_SRGB; // Para texturas de cor (diffuse)
        } else if (channels == 4) {
            format = GL_RGBA;
            internalFormat = GL_SRGB_ALPHA;
        }

        // Para texturas que não são de cor (normal, roughness, etc), usar formato linear
        if (type != TextureType::DIFFUSE && type != TextureType::EMISSION) {
            if (channels == 3) internalFormat = GL_RGB;
            if (channels == 4) internalFormat = GL_RGBA;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, 
                     format, GL_UNSIGNED_BYTE, data);

        if (params.generateMipmap) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        // Configurar parâmetros de wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)params.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)params.wrapT);

        // Configurar parâmetros de filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)params.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)params.magFilter);

        stbi_image_free(data);
        loaded = true;

        std::cout << "Textura carregada: " << filepath 
                  << " (" << width << "x" << height << ", " 
                  << channels << " canais)" << std::endl;

        return true;
    }

    bool LoadFromMemory(unsigned char* data, int length, TextureType texType,
                        const TextureParams& params = TextureParams()) {
        type = texType;
        stbi_set_flip_vertically_on_load(params.flipVertically);

        unsigned char* imageData = stbi_load_from_memory(data, length, 
                                                          &width, &height, &channels, 0);
        
        if (!imageData) {
            std::cerr << "Falha ao carregar textura da memória" << std::endl;
            return false;
        }

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
        GLenum internalFormat = channels == 4 ? GL_SRGB_ALPHA : GL_SRGB;

        if (type != TextureType::DIFFUSE && type != TextureType::EMISSION) {
            internalFormat = format;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                     format, GL_UNSIGNED_BYTE, imageData);

        if (params.generateMipmap) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)params.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)params.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)params.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)params.magFilter);

        stbi_image_free(imageData);
        loaded = true;

        std::cout << "Textura carregada da memória: " 
                  << width << "x" << height << std::endl;

        return true;
    }

    void Bind(unsigned int slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    void Unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void setId(unsigned int v) { id = v; }
    void setPath(const std::string& p) { path = p; }
    void setType(TextureType t) { type = t; }

    unsigned int GetID() const { return id; }
    const std::string& GetPath() const { return path; }
    TextureType GetType() const { return type; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetChannels() const { return channels; }
    bool IsLoaded() const { return loaded; }

    // Prevenir cópia
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Permitir movimentação
    Texture(Texture&& other) noexcept
        : id(other.id), path(std::move(other.path)), type(other.type),
          width(other.width), height(other.height), channels(other.channels),
          loaded(other.loaded) {
        other.loaded = false;
        other.id = 0;
    }

    Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            if (loaded) {
                glDeleteTextures(1, &id);
            }
            id = other.id;
            path = std::move(other.path);
            type = other.type;
            width = other.width;
            height = other.height;
            channels = other.channels;
            loaded = other.loaded;
            other.loaded = false;
            other.id = 0;
        }
        return *this;
    }
};

// Gerenciador de texturas com cache
class TextureManager {
private:
    std::map<std::string, std::shared_ptr<Texture>> cache;
    
    static TextureManager* instance;
    TextureManager() {}

public:
    static TextureManager& GetInstance() {
        if (!instance) {
            instance = new TextureManager();
        }
        return *instance;
    }

    std::shared_ptr<Texture> LoadTexture(const std::string& path, 
                                         TextureType type,
                                         const TextureParams& params = TextureParams()) {
        // Verificar cache
        auto it = cache.find(path);
        if (it != cache.end()) {
            std::cout << "Textura encontrada no cache: " << path << std::endl;
            return it->second;
        }

        // Carregar nova textura
        auto texture = std::make_shared<Texture>();
        if (texture->LoadFromFile(path, type, params)) {
            cache[path] = texture;
            return texture;
        }

        return nullptr;
    }

    void ClearCache() {
        cache.clear();
        std::cout << "Cache de texturas limpo" << std::endl;
    }

    size_t GetCacheSize() const {
        return cache.size();
    }

    void PrintCacheInfo() const {
        std::cout << "\n=== Cache de Texturas ===" << std::endl;
        std::cout << "Total: " << cache.size() << " texturas" << std::endl;
        for (const auto& pair : cache) {
            std::cout << "- " << pair.first << std::endl;
        }
        std::cout << "========================\n" << std::endl;
    }
};

// Inicializar instância estática
TextureManager* TextureManager::instance = nullptr;

// Helper para conversão de tipo para string (para shaders)
inline std::string TextureTypeToString(TextureType type) {
    switch (type) {
        case TextureType::DIFFUSE: return "texture_diffuse";
        case TextureType::SPECULAR: return "texture_specular";
        case TextureType::NORMAL: return "texture_normal";
        case TextureType::HEIGHT: return "texture_height";
        case TextureType::AMBIENT: return "texture_ambient";
        case TextureType::EMISSION: return "texture_emission";
        case TextureType::METALLIC: return "texture_metallic";
        case TextureType::ROUGHNESS: return "texture_roughness";
        case TextureType::AO: return "texture_ao";
        default: return "texture_unknown";
    }
}

inline TextureType TextureTypeFromString(const std::string& s) {
    if (s == "texture_diffuse") return TextureType::DIFFUSE;
    if (s == "texture_specular") return TextureType::SPECULAR;
    if (s == "texture_normal" || s == "texture_normals") return TextureType::NORMAL;
    if (s == "texture_height") return TextureType::HEIGHT;
    if (s == "texture_ambient") return TextureType::AMBIENT;
    if (s == "texture_emission") return TextureType::EMISSION;
    if (s == "texture_metallic") return TextureType::METALLIC;
    if (s == "texture_roughness") return TextureType::ROUGHNESS;
    if (s == "texture_ao") return TextureType::AO;
    return TextureType::UNKNOWN;
}

#endif // TEXTURE_HPP