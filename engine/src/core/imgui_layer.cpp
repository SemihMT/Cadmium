#include <cadmium/core/cadmium_theme.hpp>
#include <cadmium/core/imgui_layer.hpp>
#include <cadmium/render/renderer.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_impl_wgpu.h>
#endif

namespace Cadmium
{
    void
    ImGuiLayer::InitForSDLRenderer(SDL_Window* window, SDL_Renderer* renderer, IRenderer& backend)
    {
#ifdef CADMIUM_IMGUI
        m_Backend = RendererBackend::SDL2D;
        m_SDLRenderer = renderer;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();
        Cadmium::ApplyCadmiumTheme();

        ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer3_Init(renderer);

        backend.SetImGuiBeginHook([this] { Begin(); });
        backend.SetImGuiRenderHook([this](void* handle) { RenderDrawData(handle); });
#endif
    }

    void ImGuiLayer::InitForWebGPU(SDL_Window* window,
                                   WGPUDevice device,
                                   WGPUTextureFormat surfaceFormat,
                                   IRenderer& backend)
    {
#ifdef CADMIUM_IMGUI
        m_Backend = RendererBackend::WebGPU;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();
        Cadmium::ApplyCadmiumTheme();

        // No renderer-specific platform requirements (unlike Vulkan/D3D) - InitForOther
        // just wires up input/windowing.
        ImGui_ImplSDL3_InitForOther(window);

        ImGui_ImplWGPU_InitInfo initInfo{};
        initInfo.Device = device;
        initInfo.NumFramesInFlight = 3;
        initInfo.RenderTargetFormat = surfaceFormat;
        initInfo.DepthStencilFormat = WGPUTextureFormat_Undefined;
        ImGui_ImplWGPU_Init(&initInfo);

        backend.SetImGuiBeginHook([this] { Begin(); });
        backend.SetImGuiRenderHook([this](void* handle) { RenderDrawData(handle); });
#endif
    }

    void ImGuiLayer::Shutdown()
    {
#ifdef CADMIUM_IMGUI
        if (m_Backend == RendererBackend::SDL2D)
            ImGui_ImplSDLRenderer3_Shutdown();
        else
            ImGui_ImplWGPU_Shutdown();

        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
#endif
    }

    void ImGuiLayer::Begin()
    {
#ifdef CADMIUM_IMGUI
        if (m_Backend == RendererBackend::SDL2D)
            ImGui_ImplSDLRenderer3_NewFrame();
        else
            ImGui_ImplWGPU_NewFrame();

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
#endif
    }

    void ImGuiLayer::RenderDrawData(void* backendHandle)
    {
#ifdef CADMIUM_IMGUI
        ImGui::Render();
        if (m_Backend == RendererBackend::WebGPU)
            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(),
                                          static_cast<WGPURenderPassEncoder>(backendHandle));
        else
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_SDLRenderer);
#endif
    }
    void ImGuiLayer::ProcessEvent(SDL_Event& event)
    {
#ifdef CADMIUM_IMGUI
        ImGui_ImplSDL3_ProcessEvent(&event);
#endif
    }
} // namespace Cadmium
