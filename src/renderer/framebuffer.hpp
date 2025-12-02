#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include <GL/glew.h>
#include <iostream>

class FrameBuffer
{
private:
    unsigned int framebuffer;
    unsigned int textureColorbuffer;
    unsigned int rbo;
    int width;
    int height;
    bool initialized;

public:
    FrameBuffer(int w = 800, int h = 600) 
        : framebuffer(0), textureColorbuffer(0), rbo(0), 
          width(w), height(h), initialized(false) {
    }

    ~FrameBuffer() {
        Cleanup();
    }

    bool Init() {
        if (initialized) {
            std::cerr << "Framebuffer já foi inicializado!" << std::endl;
            return false;
        }

        // Create Framebuffer
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // Create texturo to the framebuffer
        glGenTextures(1, &textureColorbuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
        
        // Create renderbuffer to depth and stencil
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        // Check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Erro: Framebuffer não está completo!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        initialized = true;
        
        std::cout << "Framebuffer initialized successfully (" << width << "x" << height << ")" << std::endl;
        return true;
    }

    void Bind() {
        if (!initialized) {
            std::cerr << "Error: Trying to use uninitialized framebuffer!" << std::endl;
            return;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, width, height);
    }

    void Unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Resize(int w, int h) {
        if (!initialized) return;
        
        width = w;
        height = h;
        
        // Resize texture
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        
        // Resize renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        
        std::cout << "Framebuffer resized to " << width << "x" << height << std::endl;
    }

    unsigned int GetTexture() const {
        return textureColorbuffer;
    }

    unsigned int GetFramebufferId() const {
        return framebuffer;
    }

    void Cleanup() {
        if (initialized) {
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &textureColorbuffer);
            glDeleteRenderbuffers(1, &rbo);
            initialized = false;
            std::cout << "Framebuffer cleaned up successfully" << std::endl;
        }
    }

    // Prevenir cópia
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    
    // Permitir movimentação
    FrameBuffer(FrameBuffer&& other) noexcept
        : framebuffer(other.framebuffer),
          textureColorbuffer(other.textureColorbuffer),
          rbo(other.rbo),
          width(other.width),
          height(other.height),
          initialized(other.initialized) {
        other.initialized = false;
    }

    FrameBuffer& operator=(FrameBuffer&& other) noexcept {
        if (this != &other) {
            Cleanup();
            framebuffer = other.framebuffer;
            textureColorbuffer = other.textureColorbuffer;
            rbo = other.rbo;
            width = other.width;
            height = other.height;
            initialized = other.initialized;
            other.initialized = false;
        }
        return *this;
    }
};

#endif // FRAMEBUFFER_HPP
