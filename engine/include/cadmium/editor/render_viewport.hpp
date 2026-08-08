#ifndef CADMIUM_EDITOR_RENDER_VIEWPORT_HPP
#define CADMIUM_EDITOR_RENDER_VIEWPORT_HPP

#include "cadmium/render/webgpu_renderer.hpp"
#include "imgui.h"
#include "webgpu/webgpu.h"
#include <SDL3/SDL.h>
#include <cadmium/core/logger.hpp>
#include <imgui_impl_wgpu.h>

#include <memory>

namespace Cadmium::Editor
{

    struct SdlTextureDeleter
    {
        void operator()(SDL_Texture* tex) const noexcept { SDL_DestroyTexture(tex); }
    };

    // Owns an SDL_Texture with SDL_TEXTUREACCESS_TARGET.
    // The engine renders into it; the editor panel displays it via ImGui::Image.
    class RenderViewport
    {
      public:
        RenderViewport() = default;
        ~RenderViewport()
        {
            if (m_WebGPURenderer)
                m_WebGPURenderer->ClearViewportRenderTarget();
            DestroyWebGPU();
        }

        RenderViewport(RenderViewport&&) noexcept = delete;
        RenderViewport& operator=(RenderViewport&&) noexcept = delete;
        RenderViewport(const RenderViewport&) = delete;
        RenderViewport& operator=(const RenderViewport&) = delete;

        // Call after SDL_Renderer is available.
        // width/height = initial size in pixels.
        bool InitSDL(SDL_Renderer* renderer, int width, int height)
        {
            SDL_assert(m_Renderer == nullptr);
            m_Renderer = renderer;
            return ResizeSDL(width, height);
        }
        bool InitWebGPU(WebGPURenderer& renderer, int width, int height)
        {
            m_WebGPURenderer = &renderer;
            return ResizeWebGPU(width, height);
        }

        bool Resize(int width, int height)
        {
            if (m_Renderer)
                return ResizeSDL(width, height);

            if (m_WebGPURenderer)
                return ResizeWebGPU(width, height);

            return false;
        }

        bool Bind() const
        {
            if(m_Renderer)
                return BindSDL();
            if(m_WebGPURenderer)
                return BindWebGPU();
            return false;
        }
        bool Unbind() const
        {
            if(m_Renderer)
                return UnbindSDL();
            if(m_WebGPURenderer)
                return UnbindWebGPU();
            return false;
        }

        bool ResizeSDL(int width, int height)
        {
            if (width <= 0 || height <= 0)
                return false;
            if (!m_Renderer)
            {
                Log::Warn("RenderViewport", "Resize called without a renderer.");
                return false;
            }

            if (m_Texture && width == m_Width && height == m_Height)
                return true;

            m_Texture.reset();

            std::unique_ptr<SDL_Texture, SdlTextureDeleter> newTex(SDL_CreateTexture(
                m_Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height));

            if (!newTex)
            {
                Log::Error(
                    "[RenderViewport]", "Failed to create render target: {}", SDL_GetError());
                m_Width = 0;
                m_Height = 0;
                return false;
            }

            m_Texture = std::move(newTex);
            m_Width = width;
            m_Height = height;
            return true;
        }
        bool ResizeWebGPU(int width, int height)
        {
            if (width <= 0 || height <= 0)
                return false;
            if (!m_WebGPURenderer)
            {
                Log::Warn("RenderViewport", "ResizeWebGPU called without a WebGPU renderer.");
                return false;
            }

            if (m_WgpuTexture && width == m_Width && height == m_Height)
                return true;

            DestroyWebGPU();
            m_WebGPURenderer->ClearViewportRenderTarget();

            WGPUTextureDescriptor desc{};
            desc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            desc.format = m_WebGPURenderer->GetSurfaceFormat();
            desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
            desc.dimension = WGPUTextureDimension_2D;
            desc.mipLevelCount = 1;
            desc.sampleCount = 1;

            m_WgpuTexture = wgpuDeviceCreateTexture(m_WebGPURenderer->GetDevice(), &desc);
            if (!m_WgpuTexture)
            {
                Log::Error("RenderViewport", "Failed to create WebGPU viewport texture.");

                m_Width = 0;
                m_Height = 0;

                return false;
            }

            WGPUTextureViewDescriptor viewDesc{};
            viewDesc.format = desc.format;
            viewDesc.dimension = WGPUTextureViewDimension_2D;
            viewDesc.mipLevelCount = 1;
            viewDesc.arrayLayerCount = 1;
            viewDesc.aspect = WGPUTextureAspect_All;
            m_WgpuView = wgpuTextureCreateView(m_WgpuTexture, &viewDesc);

            if (!m_WgpuView)
            {
                Log::Error("RenderViewport", "Failed to create WebGPU viewport texture view.");
                DestroyWebGPU();
                m_WebGPURenderer->ClearViewportRenderTarget();
                m_Width = 0;
                m_Height = 0;
                return false;
            }

            m_WebGPURenderer->SetViewportRenderTarget(m_WgpuView);
            m_Width = width;
            m_Height = height;
            return true;
        }

        // Redirect SDL rendering to this texture.
        bool BindSDL() const
        {
            if (!m_Renderer || !m_Texture)
                return false;

            if (!SDL_SetRenderTarget(m_Renderer, m_Texture.get()))
            {
                Log::Error("RenderViewport", " Bind failed: {}", SDL_GetError());
                return false;
            }
            SDL_RenderClear(m_Renderer);

            return true;
        }
        bool BindWebGPU() const
        {
            return m_WgpuView != nullptr;
        }

        // Restore rendering to the default backbuffer.
        bool UnbindSDL() const
        {
            if (!m_Renderer)
                return false;
            if (!SDL_SetRenderTarget(m_Renderer, nullptr))
            {
                Log::Error("RenderViewport", "Unbind failed: {}", SDL_GetError());
                return false;
            }
            return true;
        }
        bool UnbindWebGPU() const
        {
            return true;
        }

        SDL_Texture* GetSDLTexture() const { return m_Texture.get(); }
        WGPUTextureView GetWebGPUTextureView() const { return m_WgpuView; }
        ImTextureID  GetImTextureID() const
        {
            if (m_WgpuView)
                return reinterpret_cast<ImTextureID>(m_WgpuView);

            return reinterpret_cast<ImTextureID>(m_Texture.get());
        }
        ImTextureID GetWebGPUImTextureID() const
        {
            return reinterpret_cast<ImTextureID>(m_WgpuView);
        }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        void SetScreenPos(float x, float y)
        {
            m_ScreenX = x;
            m_ScreenY = y;
        }
        float GetScreenX() const { return m_ScreenX; }
        float GetScreenY() const { return m_ScreenY; }
        bool IsReady() const { return m_Texture || m_WgpuView; }

      private:
        void DestroyWebGPU()
        {
            if (m_WgpuView)
            {
                wgpuTextureViewRelease(m_WgpuView);
                m_WgpuView = nullptr;
            }

            if (m_WgpuTexture)
            {
                wgpuTextureRelease(m_WgpuTexture);
                m_WgpuTexture = nullptr;
            }
        }

      private:
        SDL_Renderer* m_Renderer{nullptr};
        std::unique_ptr<SDL_Texture, SdlTextureDeleter> m_Texture;

        WebGPURenderer* m_WebGPURenderer{nullptr};
        WGPUTexture m_WgpuTexture{nullptr};
        WGPUTextureView m_WgpuView{nullptr};

        int m_Width{0};
        int m_Height{0};
        float m_ScreenX{0.f};
        float m_ScreenY{0.f};
    };

} // namespace Cadmium::Editor
#endif // CADMIUM_EDITOR_RENDER_VIEWPORT_HPP
