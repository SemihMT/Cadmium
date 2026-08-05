#ifndef CADMIUM_WEBGPU_RENDERER_HPP
#define CADMIUM_WEBGPU_RENDERER_HPP

#include <SDL3/SDL.h>
#include <cadmium/render/renderer.hpp>
#include <functional>
#include <webgpu/webgpu_cpp.h>

namespace Cadmium
{
    // WebGPU-backed IRenderer. Step 1: device/surface bootstrap and a
    // clear-to-color EndFrame only. Draw*/texture methods are stubs -
    // wired up in step 2/3.
    class WebGPURenderer : public IRenderer
    {
      public:
        WebGPURenderer(SDL_Window* window, int width, int height);
        ~WebGPURenderer() override;

        WebGPURenderer(const WebGPURenderer&) = delete;
        WebGPURenderer& operator=(const WebGPURenderer&) = delete;

        // Kicks off instance -> adapter -> device -> surface-configure.
        // onComplete(true) once the device is usable; onComplete(false)
        // on failure (e.g. browser without WebGPU support).
        void RequestDevice(std::function<void(bool)> onComplete);

        void BeginFrame() override;
        void EndFrame() override;

        void DrawLine(const DrawCmd::Line&) override {}
        void DrawRect(const DrawCmd::Rect&) override {}
        void DrawCircle(const DrawCmd::Circle&) override {}
        void DrawPolygon(const DrawCmd::Polygon&) override {}
        void DrawText(const DrawCmd::Text&) override {}
        void DrawSprite(const DrawCmd::Sprite&) override {}
        void SetCamera(const DrawCmd::SetCamera&) override {}
        void ResetCamera(const DrawCmd::ResetCamera&) override {}
        void DrawFullscreenTexture(TextureHandle) override {}

        TextureHandle CreateTextureFromFile(const std::string&) override
        {
            return k_InvalidTexture;
        }
        TextureDesc GetTextureDesc(TextureHandle) const override { return {}; }
        void DestroyTexture(TextureHandle) override {}

      private:
        void OnAdapterRequestEnded(WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter,
                                   WGPUStringView message);
        void OnDeviceRequestEnded(WGPURequestDeviceStatus status,
                             WGPUDevice device,
                             WGPUStringView message);
        void OnAdapterReady(WGPUAdapter adapter);
        void OnDeviceReady(WGPUDevice device);
        void CreateSurface();
        void ConfigureSurface();

        void OnDeviceLost(WGPUDevice const* device,
                                   WGPUDeviceLostReason reason,
                                   WGPUStringView message);

        SDL_Window* m_Window{ nullptr };
        int m_Width, m_Height;
        std::function<void(bool)> m_OnReady;

        WGPUInstance m_Instance{nullptr};
        WGPUAdapter m_Adapter{nullptr};
        WGPUDevice m_Device{nullptr};
        WGPUQueue m_Queue{nullptr};
        WGPUSurface m_Surface{nullptr};
        WGPUTextureFormat m_SurfaceFormat{WGPUTextureFormat_Undefined};
        WGPUSurfaceTexture m_CurrentSurfaceTexture{};
    };
} // namespace Cadmium
#endif
