#include "opengl_device.hpp"
#include "src/core/window.hpp"
#include <memory>

namespace RHI {

std::unique_ptr<IDevice> DeviceFactory::Create(API api) {
    switch (api) {
        case API::OpenGL:
            return std::make_unique<OpenGLDevice>();
        
        case API::Vulkan:
            // return std::make_unique<VulkanDevice>();
            std::cerr << "[RHI] Vulkan não implementado ainda!" << std::endl;
            return nullptr;
        
        case API::DirectX12:
            // return std::make_unique<D3D12Device>();
            std::cerr << "[RHI] DirectX12 não implementado ainda!" << std::endl;
            return nullptr;
        
        case API::Metal:
            // return std::make_unique<MetalDevice>();
            std::cerr << "[RHI] Metal não implementado ainda!" << std::endl;
            return nullptr;
        
        default:
            return nullptr;
        }
    }
}

using namespace RHI;

class App {
private:
    std::unique_ptr<Window> window;
    std::unique_ptr<IDevice> device;
    std::unique_ptr<FramebufferHandle> framebuffer;

    std::string title;
    int width;
    int height;

    bool Init() {
        if (!window->Init()) 
            return false;

        window->SetResizeCallback([this](int w, int h) {
            // device->SetViewport({0, 0, window->GetWidth(), window->GetHeight(), 0.0f, 1.0f});
            // if (this->fb) this->fb->Resize(w, h);
            device->ResizeFramebuffer(*framebuffer, window->GetWidth(), window->GetHeight());
        });

        // Create Graphics Context
        device = DeviceFactory::Create(API::OpenGL);
        if (!device) {
            std::cerr << "Falha ao criar device!" << std::endl;
            return false;
        }

        // Initialize Graphics
        if (!device->Initialize()) {
            std::cerr << "Falha ao inicializar device!" << std::endl;
            return false;
        }

        return true;
    }

public:
    App(const std::string& title, int width, int height)
        : title(title), width(width), height(height) {
        window = std::make_unique<Window>(width, height, this->title);
    }

    ~App() {
        device->Shutdown();
        std::cout << "Recursos liberados com sucesso!" << std::endl;
    }

    void Run() {
        if (!Init()) return;

        FramebufferDescriptor fbDesc;
        fbDesc.width = window->GetWidth();
        fbDesc.height = window->GetHeight();
        fbDesc.colorFormats = {TextureFormat::RGBA8};
        fbDesc.hasDepth = true;

        framebuffer = std::make_unique<FramebufferHandle>(device->CreateFramebuffer(fbDesc));

        while (!window->ShouldClose()) {
            device->SetClearColor({1.0f, 0.1f, 0.15f, 1.0f});
            device->Clear(true, true, false);
            device->BindFramebuffer(*framebuffer);

            // Swap buffers and poll events
            window->OnUpdate();
        }
    }
};

int main() {
    App app("OpenGL Render", 1280, 720);
    app.Run();
    return 0;
}
