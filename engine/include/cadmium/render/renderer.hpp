#ifndef CADMIUM_RENDERER
#define CADMIUM_RENDERER

#include <cadmium/core/draw_command_queue.hpp>
#include <cadmium/core/handles.hpp>
#include <functional>
#include <string>

namespace Cadmium
{
    struct TextureDesc
    {
        int width{0};
        int height{0};
    };

    class IRenderer
    {
      public:
        virtual ~IRenderer() = default;

        virtual void BeginFrame(Cadmium::Color clearColor) = 0;
        virtual void EndFrame() = 0;
        virtual void Resize(int width, int height) {}

        virtual void DrawLine(const DrawCmd::Line&) = 0;
        virtual void DrawRect(const DrawCmd::Rect&) = 0;
        virtual void DrawCircle(const DrawCmd::Circle&) = 0;
        virtual void DrawPolygon(const DrawCmd::Polygon&) = 0;
        virtual void DrawText(const DrawCmd::Text&) = 0;
        virtual void DrawSprite(const DrawCmd::Sprite&) = 0;
        virtual void SetCamera(const DrawCmd::SetCamera&) = 0;
        virtual void ResetCamera(const DrawCmd::ResetCamera&) = 0;
        virtual void DrawFullscreenTexture(TextureHandle handle) = 0;

        // Texture lifecycle
        virtual TextureHandle CreateTextureFromFile(const std::string& path) = 0;
        virtual TextureHandle CreateTextureFromMemory(
            int width, int height, const void* pixelsRGBA8, int rowBytes = 0) = 0;
        virtual TextureDesc GetTextureDesc(TextureHandle) const = 0;
        virtual void DestroyTexture(TextureHandle) = 0;
        //Used by TextCache to blit its cached textures.
        virtual void DrawTexturedQuadScreen(
            TextureHandle handle, float screenX, float screenY, float width, float height, const Color& tint) = 0;

        // Editor Only
        // Renderer defined implementation. Should return a pointer to something that represents a texture in the used backend.
        //
        // OpenGL:       ImTextureID = GLuint                      (see ImGui_ImplOpenGL3_RenderDrawData()      in imgui_impl_opengl3.cpp)
        // DirectX9:     ImTextureID = LPDIRECT3DTEXTURE9          (see ImGui_ImplDX9_RenderDrawData()          in imgui_impl_dx9.cpp)
        // DirectX11:    ImTextureID = ID3D11ShaderResourceView*   (see ImGui_ImplDX11_RenderDrawData()         in imgui_impl_dx11.cpp)
        // DirectX12:    ImTextureID = D3D12_GPU_DESCRIPTOR_HANDLE (see ImGui_ImplDX12_RenderDrawData()         in imgui_impl_dx12.cpp)
        // SDL_Renderer: ImTextureID = SDL_Texture*                (see ImGui_ImplSDLRenderer2_RenderDrawData() in imgui_impl_sdlrenderer2.cpp)
        // Vulkan:       ImTextureID = ImageView's VkDescriptorSet (see ImGui_ImplVulkan_RenderDrawData()       in imgui_impl_vulkan.cpp)
        // WebGPU:       ImTextureID = WGPUTextureView             (see ImGui_ImplWGPU_RenderDrawData()         in imgui_impl_wgpu.cpp)
        // from: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
        virtual void* GetNativeTextureHandle(TextureHandle handle) const { return nullptr; }

        void SetImGuiBeginHook(std::function<void()> hook) {m_ImGuiBeginHook = std::move(hook);}
        void SetImGuiRenderHook(std::function<void(void*)> hook) { m_ImGuiRenderHook = std::move(hook); }

        protected:
        void InvokeImGuiBeginHook() { if (m_ImGuiBeginHook) m_ImGuiBeginHook(); }
        void InvokeImGuiHook(void* backendHandle) { if (m_ImGuiRenderHook) m_ImGuiRenderHook(backendHandle); }

        private:
        std::function<void()> m_ImGuiBeginHook;
        std::function<void(void*)> m_ImGuiRenderHook;
    };

    template <typename Cmd> void DispatchDrawCommand(IRenderer& renderer, const Cmd& cmd)
    {
        if constexpr (std::is_same_v<Cmd, DrawCmd::Line>)
            renderer.DrawLine(cmd);
        else if constexpr (std::is_same_v<Cmd, DrawCmd::Rect>)
            renderer.DrawRect(cmd);
        else if constexpr (std::is_same_v<Cmd, DrawCmd::Circle>)
            renderer.DrawCircle(cmd);
        else if constexpr (std::is_same_v<Cmd, DrawCmd::Polygon>)
            renderer.DrawPolygon(cmd);
        else if constexpr (std::is_same_v<Cmd, DrawCmd::Text>)
            renderer.DrawText(cmd);
        else if constexpr (std::is_same_v<Cmd, DrawCmd::Sprite>)
            renderer.DrawSprite(cmd);
        else if constexpr (std::is_same_v<Cmd, DrawCmd::SetCamera>)
            renderer.SetCamera(cmd);
        else if constexpr (std::is_same_v<Cmd, DrawCmd::ResetCamera>)
            renderer.ResetCamera(cmd);
        else
            static_assert(sizeof(Cmd) == 0, "Unhandled DrawCmd variant in DispatchDrawCommand");
    }
} // namespace Cadmium
#endif
