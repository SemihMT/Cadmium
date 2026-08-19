#ifndef CADMIUM_WEBGPU_RENDERER_HPP
#define CADMIUM_WEBGPU_RENDERER_HPP

#include "cadmium/render/text_cache.hpp"
#include <SDL3/SDL.h>

#include <cadmium/core/draw_command_queue.hpp>
#include <cadmium/render/renderer.hpp>

#include <functional>
#include <string>
#include <vector>

#include <webgpu/webgpu_cpp.h>


namespace Cadmium
{

    class WebGPURenderer : public IRenderer
    {
      public:
        WebGPURenderer(SDL_Window* window, int width, int height, TextCache& textCache);
        ~WebGPURenderer() override;

        WebGPURenderer(const WebGPURenderer&) = delete;
        WebGPURenderer& operator=(const WebGPURenderer&) = delete;

        // -------------------------------------------------------------------------
        // Initialization
        // -------------------------------------------------------------------------

        // Starts the instance -> adapter -> device -> surface initialization.
        //
        // onComplete(true)  is called once the device is usable.
        // onComplete(false) is called if initialization fails.
        void RequestDevice(std::function<void(bool)> onComplete);

        // -------------------------------------------------------------------------
        // Frame
        // -------------------------------------------------------------------------

        void BeginFrame(Cadmium::Color color) override;
        void EndFrame() override;

        // -------------------------------------------------------------------------
        // Drawing
        // -------------------------------------------------------------------------

        void DrawLine(const DrawCmd::Line&) override;
        void DrawRect(const DrawCmd::Rect&) override;
        void DrawCircle(const DrawCmd::Circle&) override;
        void DrawPolygon(const DrawCmd::Polygon&) override;
        void DrawText(const DrawCmd::Text&) override;
        void DrawSprite(const DrawCmd::Sprite&) override;
        void DrawFullscreenTexture(TextureHandle) override;

        void SetCamera(const DrawCmd::SetCamera&) override;
        void ResetCamera(const DrawCmd::ResetCamera&) override;
        void WorldToScreen(float worldX, float worldY, float& screenX, float& screenY) const override;
        void ScreenToWorld(float screenX, float screenY, float& worldX, float& worldY) const override;

        // -------------------------------------------------------------------------
        // Textures
        // -------------------------------------------------------------------------

        TextureHandle CreateTextureFromFile(const std::string&) override;
        TextureHandle CreateTextureFromMemory(int width,
                                              int height,
                                              const void* pixelsRGBA8,
                                              int rowBytes) override;
        TextureDesc GetTextureDesc(TextureHandle) const override;
        void DestroyTexture(TextureHandle) override;
        void DrawTexturedQuadScreen(
            TextureHandle handle, float screenX, float screenY, float width, float height, const Color& tint) override;
        void* GetNativeTextureHandle(TextureHandle handle) const override;

        // -------------------------------------------------------------------------
        // WebGPU access
        // -------------------------------------------------------------------------
        void Resize(int width, int height) override;
        WGPUDevice GetDevice() const { return m_Device; }
        WGPUTextureFormat GetSurfaceFormat() const { return m_SurfaceFormat; }
        void SetViewportRenderTarget(WGPUTextureView view, int width, int height);
        void ClearViewportRenderTarget() { m_ViewportRenderTargetView = nullptr; }

        // -------------------------------------------------------------------------
        // Pipelines
        // -------------------------------------------------------------------------

        void CreateFlatColorPipeline();
        void CreateTexturedPipeline();


      private:
        // -------------------------------------------------------------------------
        // Initialization callbacks
        // -------------------------------------------------------------------------

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

        // -------------------------------------------------------------------------
        // Geometry / batching
        // -------------------------------------------------------------------------

        struct FlatVertex
        {
            float x;
            float y;
            float r;
            float g;
            float b;
            float a;
        };
        struct TexturedVertex
        {
            float x, y; // NDC
            float u, v;
            float r, g, b, a; // tint
        };
        enum class BatchKind
        {
            Flat,
            Textured
        };
        struct DrawRun
        {
            BatchKind kind;
            uint32_t vertexOffset;
            uint32_t vertexCount;
            TextureHandle textureHandle{k_InvalidTexture}; // unused for Flat
        };
        struct StoredTexture
        {
            WGPUTexture texture{nullptr};
            WGPUTextureView view{nullptr};
            WGPUBindGroup bindGroup{nullptr};
            TextureDesc desc{};
        };
        static constexpr float k_LineThicknessPx = 1.5f;

        void ToScreen(float wx, float wy, float& sx, float& sy) const;
        void PushVertex(float sx, float sy, const Color& col);
        void PushTexturedVertex(float sx, float sy, float u, float v, const Color& col);
        void AppendQuad(float ax, float ay,
                        float bx, float by,
                        float cx, float cy,
                        float dx, float dy,
                        const Color& col);
        void PushTexturedQuad(float ax, float ay, float au, float av,
                       float bx, float by, float bu, float bv,
                       float cx, float cy, float cu, float cv,
                       float dx, float dy, float du, float dv,
                       const Color& col);

        void AppendThickLine(float x1, float y1,
                             float x2, float y2,
                             const Color& col,
                             float thickness);

        void EnsureVertexBufferCapacity(size_t requiredBytes);
        void EnsureTexturedVertexBufferCapacity(size_t requiredBytes);
        void BeginFlatRun();
        void EndFlatRun();
        void FlushDrawRuns(WGPURenderPassEncoder pass);

        TextureHandle
        UploadRGBA8Texture(int width, int height, const void* pixels, size_t rowBytes);

        // -------------------------------------------------------------------------
        // Window / frame state
        // -------------------------------------------------------------------------

        SDL_Window* m_Window{nullptr};

        int CurrentTargetWidth() const
        {
            return m_ViewportRenderTargetView ? m_ViewportWidth : m_Width;
        }
        int CurrentTargetHeight() const
        {
            return m_ViewportRenderTargetView ? m_ViewportHeight : m_Height;
        }

        int m_Width{0};
        int m_Height{0};
        int m_ViewportWidth{0};
        int m_ViewportHeight{0};
        Cadmium::Color m_ClearColor;

        std::function<void(bool)> m_OnReady;

        // -------------------------------------------------------------------------
        // Text
        // -------------------------------------------------------------------------

        TextCache& m_TextCache;

        // -------------------------------------------------------------------------
        // Camera state
        // -------------------------------------------------------------------------

        float m_CamX{0.0f};
        float m_CamY{0.0f};
        float m_CamZoom{1.0f};

        // -------------------------------------------------------------------------
        // WebGPU objects
        // -------------------------------------------------------------------------

        WGPUInstance m_Instance{nullptr};
        WGPUAdapter m_Adapter{nullptr};
        WGPUDevice m_Device{nullptr};
        WGPUQueue m_Queue{nullptr};

        WGPUSurface m_Surface{nullptr};

        // -------------------------------------------------------------------------
        // Surface / render target
        // -------------------------------------------------------------------------

        WGPUTextureFormat m_SurfaceFormat{WGPUTextureFormat_Undefined};
        WGPUTextureView m_ViewportRenderTargetView{nullptr};
        WGPUSurfaceTexture m_CurrentSurfaceTexture{};

        // -------------------------------------------------------------------------
        // Rendering resources
        // -------------------------------------------------------------------------

        std::unordered_map<TextureHandle, StoredTexture> m_Textures;
        TextureHandle m_NextHandle{1};
        std::vector<FlatVertex> m_BatchVertices;
        std::vector<TexturedVertex> m_TexturedBatchVertices;
        std::vector<DrawRun> m_DrawRuns;
        size_t m_FlatRunStart{0};

        WGPUBuffer m_VertexBuffer{nullptr};
        size_t m_VertexBufferCapacityBytes{0};
        WGPUBuffer m_TexturedVertexBuffer{nullptr};
        size_t m_TexturedVertexBufferCapacityBytes{0};

        WGPURenderPipeline m_FlatColorPipeline{nullptr};
        WGPURenderPipeline m_TexturePipeline{nullptr};
        WGPUBindGroupLayout m_TextureBindGroupLayout{nullptr};
        WGPUSampler m_DefaultSampler{nullptr};
    };
} // namespace Cadmium
#endif
