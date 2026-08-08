#ifndef CADMIUM_WEBGPU_RENDERER_HPP
#define CADMIUM_WEBGPU_RENDERER_HPP

#include <SDL3/SDL.h>
#include <cadmium/core/draw_command_queue.hpp>
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

        void BeginFrame(Cadmium::Color color) override;
        void EndFrame() override;

        void DrawLine(const DrawCmd::Line&) override;
        void DrawRect(const DrawCmd::Rect&) override;
        void DrawCircle(const DrawCmd::Circle&) override;
        void DrawPolygon(const DrawCmd::Polygon&) override;
        void DrawText(const DrawCmd::Text&) override;
        void DrawSprite(const DrawCmd::Sprite&) override;
        void SetCamera(const DrawCmd::SetCamera&) override;
        void ResetCamera(const DrawCmd::ResetCamera&) override;
        void DrawFullscreenTexture(TextureHandle) override;

        TextureHandle CreateTextureFromFile(const std::string&) override
        {
            return k_InvalidTexture;
        }
        TextureDesc GetTextureDesc(TextureHandle) const override { return {}; }
        void DestroyTexture(TextureHandle) override {}

        WGPUDevice GetDevice() const { return m_Device; }
        WGPUTextureFormat GetSurfaceFormat() const { return m_SurfaceFormat; }

        void CreateFlatColorPipeline();
        void SetViewportRenderTarget(WGPUTextureView view) { m_ViewportRenderTargetView = view; }
        void ClearViewportRenderTarget() { m_ViewportRenderTargetView = nullptr; }

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

        void
        OnDeviceLost(WGPUDevice const* device, WGPUDeviceLostReason reason, WGPUStringView message);

        struct FlatVertex
        {
            float x, y;
            float r, g, b, a;
        };
        static constexpr float k_LineThicknessPx = 1.5f;
        std::vector<FlatVertex> m_BatchVertices;
        WGPUBuffer m_VertexBuffer{nullptr};
        size_t m_VertexBufferCapacityBytes{0};
        WGPURenderPipeline m_FlatColorPipeline{nullptr};
        void ToScreen(float wx, float wy, float& sx, float& sy) const
        {
            sx = (wx - m_CamX) * m_CamZoom + m_Width * 0.5f;
            sy = (wy - m_CamY) * m_CamZoom + m_Height * 0.5f;
        }
        void PushVertex(float sx, float sy, const Color& col)
        {
            float ndcX = (sx / static_cast<float>(m_Width)) * 2.0f - 1.0f;
            float ndcY =
                1.0f - (sy / static_cast<float>(m_Height)) * 2.0f; // screen Y is down, NDC Y is up
            m_BatchVertices.push_back({ndcX, ndcY, col.r, col.g, col.b, col.a});
        }
        void AppendQuad(float ax, float ay,
                        float bx, float by,
                        float cx, float cy,
                        float dx, float dy,
                        const Color& col);
        void AppendThickLine(float x1, float y1, float x2, float y2, const Color& col, float thickness);
        void EnsureVertexBufferCapacity(size_t requiredBytes);
        void FlushBatch(WGPURenderPassEncoder pass);

        SDL_Window* m_Window{nullptr};
        Cadmium::Color m_ClearColor;
        int m_Width, m_Height;
        float m_CamX{0.f}, m_CamY{0.f}, m_CamZoom{1.f};
        std::function<void(bool)> m_OnReady;

        WGPUInstance m_Instance{nullptr};
        WGPUAdapter m_Adapter{nullptr};
        WGPUDevice m_Device{nullptr};
        WGPUQueue m_Queue{nullptr};
        WGPUSurface m_Surface{nullptr};
        WGPUTextureView m_ViewportRenderTargetView{nullptr};
        WGPUTextureFormat m_SurfaceFormat{WGPUTextureFormat_Undefined};
        WGPUSurfaceTexture m_CurrentSurfaceTexture{};
    };
} // namespace Cadmium
#endif
