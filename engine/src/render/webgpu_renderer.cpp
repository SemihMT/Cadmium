#include "SDL3_image/SDL_image.h"
#include "cadmium/core/draw_command_queue.hpp"
#include "cadmium/core/handles.hpp"
#include "cadmium/render/renderer.hpp"
#include <cadmium/core/logger.hpp>
#include <cadmium/render/native_surface.hpp>
#include <cadmium/render/webgpu_renderer.hpp>
#include <numbers>
#include <webgpu/webgpu.h>

#if defined(SDL_PLATFORM_MACOS)
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>

#elif defined(SDL_PLATFORM_IOS)
#include <Foundation/Foundation.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <UIKit/UIKit.h>

#elif defined(SDL_PLATFORM_WINDOWS)
#define NOMINMAX
#include <windows.h>
#ifdef DrawText
#undef DrawText
#endif
#endif

#include <SDL3/SDL.h>

namespace
{
    constexpr const char* kFlatColorShaderWGSL = R"(
    struct VertexIn {
        @location(0) position: vec2f,
        @location(1) color: vec4f,
    };
    struct VertexOut {
        @builtin(position) position: vec4f,
        @location(0) color: vec4f,
    };

    @vertex
    fn vs_main(in: VertexIn) -> VertexOut {
        var out: VertexOut;
        out.position = vec4f(in.position, 0.0, 1.0);
        out.color = in.color;
        return out;
    }

    @fragment
    fn fs_main(in: VertexOut) -> @location(0) vec4f {
        return in.color;
    }
    )";

    constexpr const char* kTexturedShaderWGSL = R"(
struct VertexIn {
    @location(0) position: vec2f,
    @location(1) uv: vec2f,
    @location(2) color: vec4f,
};
struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
};

@group(0) @binding(0) var uSampler: sampler;
@group(0) @binding(1) var uTexture: texture_2d<f32>;

@vertex
fn vs_main(in: VertexIn) -> VertexOut {
    var out: VertexOut;
    out.position = vec4f(in.position, 0.0, 1.0);
    out.uv = in.uv;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOut) -> @location(0) vec4f {
    return textureSample(uTexture, uSampler, in.uv) * in.color;
}
)";
} // namespace

namespace Cadmium
{
    WebGPURenderer::WebGPURenderer(SDL_Window* window, int width, int height)
        : m_Window{window}, m_Width(width), m_Height(height)
    {
        SDL_GetWindowSizeInPixels(m_Window, &m_Width, &m_Height);
#ifdef CADMIUM_PLATFORM_WEB
        m_Instance = wgpuCreateInstance(nullptr);
#else
        WGPUInstanceDescriptor desc = {};
        desc.nextInChain = nullptr;
        m_Instance = wgpuCreateInstance(&desc);
#endif
    }

    WebGPURenderer::~WebGPURenderer()
    {
        for (auto& [handle, tex] : m_Textures)
        {
            wgpuBindGroupRelease(tex.bindGroup);
            wgpuTextureViewRelease(tex.view);
            wgpuTextureRelease(tex.texture);
        }
        if (m_Surface)
            wgpuSurfaceRelease(m_Surface);
        if (m_Queue)
            wgpuQueueRelease(m_Queue);
        if (m_Device)
            wgpuDeviceRelease(m_Device);
        if (m_Adapter)
            wgpuAdapterRelease(m_Adapter);
        if (m_Instance)
            wgpuInstanceRelease(m_Instance);
    }

    void WebGPURenderer::RequestDevice(std::function<void(bool)> onComplete)
    {
        m_OnReady = std::move(onComplete);

        WGPURequestAdapterOptions options{};

        WGPURequestAdapterCallbackInfo callbackInfo{};
        callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
        callbackInfo.callback = [](WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter,
                                   WGPUStringView message,
                                   void* userdata1,
                                   void* /*userdata2*/)
        {
            static_cast<WebGPURenderer*>(userdata1)->OnAdapterRequestEnded(
                status, adapter, message);
        };
        callbackInfo.userdata1 = this;
        callbackInfo.userdata2 = nullptr;

        wgpuInstanceRequestAdapter(m_Instance, &options, callbackInfo);
    }

    void WebGPURenderer::OnAdapterRequestEnded(WGPURequestAdapterStatus status,
                                               WGPUAdapter adapter,
                                               WGPUStringView message)
    {
        if (status != WGPURequestAdapterStatus_Success)
        {
            std::string msg = message.data ? std::string(message.data, message.length)
                                           : std::string("adapter request failed");
            Cadmium::Log::Error("WebGPURenderer", "{}", msg);
            m_OnReady(false);
            return;
        }
        OnAdapterReady(adapter);
    }
    // New helper method – handle device request result
    void WebGPURenderer::OnDeviceRequestEnded(WGPURequestDeviceStatus status,
                                              WGPUDevice device,
                                              WGPUStringView message)
    {
        if (status != WGPURequestDeviceStatus_Success)
        {
            std::string msg =
                message.data ? std::string(message.data, message.length) : "device request failed";
            Cadmium::Log::Error("WebGPURenderer", "{}", msg);
            m_OnReady(false);
            return;
        }
        OnDeviceReady(device);
    }

    // Updated OnAdapterReady to use the same callback‑info pattern
    void WebGPURenderer::OnAdapterReady(WGPUAdapter adapter)
    {
        m_Adapter = adapter;

        WGPUDeviceDescriptor desc{};
        desc.label = {"Cadmium WebGPU Device", WGPU_STRLEN};

        WGPUDeviceLostCallbackInfo lostCallbackInfo{};
        lostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
        lostCallbackInfo.callback = [](WGPUDevice const* device,
                                       WGPUDeviceLostReason reason,
                                       WGPUStringView message,
                                       void* userdata1,
                                       void* /*userdata2*/)
        { static_cast<WebGPURenderer*>(userdata1)->OnDeviceLost(device, reason, message); };
        lostCallbackInfo.userdata1 = this;
        lostCallbackInfo.userdata2 = nullptr;
        desc.deviceLostCallbackInfo = lostCallbackInfo;

        WGPURequestDeviceCallbackInfo callbackInfo{};
        callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
        callbackInfo.callback = [](WGPURequestDeviceStatus status,
                                   WGPUDevice device,
                                   WGPUStringView message,
                                   void* userdata1,
                                   void* /*userdata2*/)
        { static_cast<WebGPURenderer*>(userdata1)->OnDeviceRequestEnded(status, device, message); };
        callbackInfo.userdata1 = this;
        callbackInfo.userdata2 = nullptr;

        wgpuAdapterRequestDevice(m_Adapter, &desc, callbackInfo);
    }

    void WebGPURenderer::OnDeviceReady(WGPUDevice device)
    {
        m_Device = device;
        m_Queue = wgpuDeviceGetQueue(m_Device);

#ifdef __EMSCRIPTEN__
        WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
        canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
        canvasDesc.selector = {"canvas", WGPU_STRLEN};

        WGPUSurfaceDescriptor surfaceDesc{};
        surfaceDesc.nextInChain = &canvasDesc.chain;
        surfaceDesc.label = {"Cadmium Surface", WGPU_STRLEN};
        m_Surface = wgpuInstanceCreateSurface(m_Instance, &surfaceDesc);
#else
        m_Surface = CreateWGPUSurfaceFromSDLWindow(m_Instance, m_Window);
#endif

        WGPUSurfaceCapabilities caps{};
        wgpuSurfaceGetCapabilities(m_Surface, m_Adapter, &caps);
        m_SurfaceFormat = caps.formats[0];

        ConfigureSurface();

        m_OnReady(true);
    }

    void WebGPURenderer::ConfigureSurface()
    {
        WGPUSurfaceConfiguration config{};
        config.device = m_Device;
        config.format = m_SurfaceFormat;
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.width = static_cast<uint32_t>(m_Width);
        config.height = static_cast<uint32_t>(m_Height);
        config.presentMode = WGPUPresentMode_Fifo;
        wgpuSurfaceConfigure(m_Surface, &config);
    }
    void WebGPURenderer::OnDeviceLost(WGPUDevice const* device,
                                      WGPUDeviceLostReason reason,
                                      WGPUStringView message)
    {
        std::string msg =
            message.data ? std::string(message.data, message.length) : std::string("(no message)");

        // WGPUDeviceLostReason_Destroyed fires on our OWN intentional teardown
        // (wgpuDeviceRelease in ~WebGPURenderer)
        if (reason == WGPUDeviceLostReason_Destroyed)
        {
            Cadmium::Log::Info("WebGPURenderer", "Device lost (intentional teardown): {}", msg);
            return;
        }

        Cadmium::Log::Error("WebGPURenderer",
                            "Device lost unexpectedly (reason={}): {}",
                            static_cast<int>(reason),
                            msg);
    }

    void WebGPURenderer::BeginFrame(Cadmium::Color clearColor)
    {
        m_ClearColor = clearColor;
        wgpuSurfaceGetCurrentTexture(m_Surface, &m_CurrentSurfaceTexture);
        InvokeImGuiBeginHook();
    }

    void WebGPURenderer::EndFrame()
    {
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_Device, nullptr);

        if (m_ViewportRenderTargetView)
        {
            // Offscreen game pass -> viewport texture.
            WGPURenderPassColorAttachment att{};
            att.view = m_ViewportRenderTargetView;
            att.loadOp = WGPULoadOp_Clear;
            att.storeOp = WGPUStoreOp_Store;
            att.clearValue = {m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a};
            att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

            WGPURenderPassDescriptor desc{};
            desc.colorAttachmentCount = 1;
            desc.colorAttachments = &att;

            WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
            FlushDrawRuns(pass);
            wgpuRenderPassEncoderEnd(pass);
            wgpuRenderPassEncoderRelease(pass);
        }

        // Chrome/main pass -> surface. Always runs, viewport or not.
        wgpuSurfaceGetCurrentTexture(m_Surface, &m_CurrentSurfaceTexture);

        WGPUTextureViewDescriptor viewDesc{};
        viewDesc.format = m_SurfaceFormat;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.mipLevelCount = 1;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;
        WGPUTextureView surfaceView =
            wgpuTextureCreateView(m_CurrentSurfaceTexture.texture, &viewDesc);

        WGPURenderPassColorAttachment att2{};
        att2.view = surfaceView;
        att2.loadOp = WGPULoadOp_Clear;
        att2.storeOp = WGPUStoreOp_Store;
        att2.clearValue = {m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a};
        att2.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor desc2{};
        desc2.colorAttachmentCount = 1;
        desc2.colorAttachments = &att2;

        WGPURenderPassEncoder pass2 = wgpuCommandEncoderBeginRenderPass(encoder, &desc2);
        if (!m_ViewportRenderTargetView)
        {
            FlushDrawRuns(pass2);
        }
        InvokeImGuiHook(pass2);
        wgpuRenderPassEncoderEnd(pass2);
        wgpuRenderPassEncoderRelease(pass2);

        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(m_Queue, 1, &cmd);

        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(surfaceView);

#ifndef CADMIUM_PLATFORM_WEB
        wgpuSurfacePresent(m_Surface);
#endif
    }

    void WebGPURenderer::DrawLine(const DrawCmd::Line& l)
    {
        BeginFlatRun();
        float x1, y1, x2, y2;
        ToScreen(l.x1, l.y1, x1, y1);
        ToScreen(l.x2, l.y2, x2, y2);
        AppendThickLine(x1, y1, x2, y2, l.color, k_LineThicknessPx);
        EndFlatRun();
    }

    void WebGPURenderer::DrawRect(const DrawCmd::Rect& r)
    {
        BeginFlatRun();
        float x, y;
        ToScreen(r.x, r.y, x, y);
        float hw = r.w * 0.5f * m_CamZoom, hh = r.h * 0.5f * m_CamZoom;
        float tlX = x - hw, tlY = y - hh, trX = x + hw, trY = y - hh;
        float brX = x + hw, brY = y + hh, blX = x - hw, blY = y + hh;

        if (r.filled)
        {
            AppendQuad(tlX, tlY, trX, trY, brX, brY, blX, blY, r.color);
        }
        else
        {
            AppendThickLine(tlX, tlY, trX, trY, r.color, k_LineThicknessPx);
            AppendThickLine(trX, trY, brX, brY, r.color, k_LineThicknessPx);
            AppendThickLine(brX, brY, blX, blY, r.color, k_LineThicknessPx);
            AppendThickLine(blX, blY, tlX, tlY, r.color, k_LineThicknessPx);
        }
        EndFlatRun();
    }

    void WebGPURenderer::DrawCircle(const DrawCmd::Circle& c)
    {
        BeginFlatRun();
        float cx, cy;
        ToScreen(c.x, c.y, cx, cy);
        float radius = c.radius * m_CamZoom;
        int segments = c.segments > 0 ? c.segments : std::max(12, static_cast<int>(radius * 0.5f));

        if (c.filled)
        {
            float prevX = cx + radius, prevY = cy;
            for (int i = 1; i <= segments; ++i)
            {
                float t = (static_cast<float>(i) / segments) * 2.0f * std::numbers::pi_v<float>;
                float nx = cx + std::cos(t) * radius, ny = cy + std::sin(t) * radius;
                // Triangle fan from center
                PushVertex(cx, cy, c.color);
                PushVertex(prevX, prevY, c.color);
                PushVertex(nx, ny, c.color);
                prevX = nx;
                prevY = ny;
            }
        }
        else
        {
            float prevX = cx + radius, prevY = cy;
            for (int i = 1; i <= segments; ++i)
            {
                float t = (static_cast<float>(i) / segments) * 2.0f * std::numbers::pi_v<float>;
                float nx = cx + std::cos(t) * radius, ny = cy + std::sin(t) * radius;
                AppendThickLine(prevX, prevY, nx, ny, c.color, k_LineThicknessPx);
                prevX = nx;
                prevY = ny;
            }
        }
        EndFlatRun();
    }

    void WebGPURenderer::DrawPolygon(const DrawCmd::Polygon& p)
    {
        BeginFlatRun();
        if (p.points.size() < 2)
            return;

        std::vector<std::pair<float, float>> screen;
        screen.reserve(p.points.size());
        for (auto& pt : p.points)
        {
            float sx, sy;
            ToScreen(pt.x, pt.y, sx, sy);
            screen.emplace_back(sx, sy);
        }

        if (p.filled)
        {
            for (size_t i = 1; i + 1 < screen.size(); ++i)
            {
                PushVertex(screen[0].first, screen[0].second, p.color);
                PushVertex(screen[i].first, screen[i].second, p.color);
                PushVertex(screen[i + 1].first, screen[i + 1].second, p.color);
            }
        }
        else
        {
            for (size_t i = 0; i < screen.size(); ++i)
            {
                size_t next = (i + 1) % screen.size();
                AppendThickLine(screen[i].first,

                                screen[i].second,
                                screen[next].first,
                                screen[next].second,
                                p.color,
                                k_LineThicknessPx);
            }
        }
        EndFlatRun();
    }
    void WebGPURenderer::DrawText(const DrawCmd::Text&) {}
    void WebGPURenderer::DrawSprite(const DrawCmd::Sprite& s)
    {
        auto it = m_Textures.find(s.textureHandle);
        if (it == m_Textures.end())
            return;

        const StoredTexture& tex = it->second;

        bool hasSrcRect = s.srcW > 0.f && s.srcH > 0.f;

        float naturalW = hasSrcRect ? s.srcW : static_cast<float>(tex.desc.width);
        float naturalH = hasSrcRect ? s.srcH : static_cast<float>(tex.desc.height);
        float w = (s.w > 0.0f ? s.w : naturalW) * m_CamZoom;
        float h = (s.h > 0.0f ? s.h : naturalH) * m_CamZoom;

        float cx, cy;
        ToScreen(s.x, s.y, cx, cy);

        float hw = w * 0.5f, hh = h * 0.5f;
        float rad = s.rotation * (std::numbers::pi_v<float> / 180.0f);
        float cs = std::cos(rad), sn = std::sin(rad);

        float lx[4] = {-hw, hw, hw, -hw};
        float ly[4] = {-hh, -hh, hh, hh};
        float sx[4], sy[4];
        for (int i = 0; i < 4; ++i)
        {
            sx[i] = cx + lx[i] * cs - ly[i] * sn;
            sy[i] = cy + lx[i] * sn + ly[i] * cs;
        }

        float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
        if (hasSrcRect && tex.desc.width > 0 && tex.desc.height > 0)
        {
            u0 = s.srcX / static_cast<float>(tex.desc.width);
            v0 = s.srcY / static_cast<float>(tex.desc.height);
            u1 = (s.srcX + s.srcW) / static_cast<float>(tex.desc.width);
            v1 = (s.srcY + s.srcH) / static_cast<float>(tex.desc.height);
        }
        if (s.flipX)
            std::swap(u0, u1);
        if (s.flipY)
            std::swap(v0, v1);

        if (m_DrawRuns.empty() || m_DrawRuns.back().kind != BatchKind::Textured ||
            m_DrawRuns.back().textureHandle != s.textureHandle)
            m_DrawRuns.push_back({BatchKind::Textured,
                                  static_cast<uint32_t>(m_TexturedBatchVertices.size()),
                                  0,
                                  s.textureHandle});

        PushTexturedQuad(sx[0],
                         sy[0],
                         u0,
                         v0,
                         sx[1],
                         sy[1],
                         u1,
                         v0,
                         sx[2],
                         sy[2],
                         u1,
                         v1,
                         sx[3],
                         sy[3],
                         u0,
                         v1,
                         s.color);

        m_DrawRuns.back().vertexCount += 6;
    }
    void WebGPURenderer::DrawFullscreenTexture(TextureHandle handle)
    {
        auto it = m_Textures.find(handle);
        if (it == m_Textures.end())
            return;

        Color white{1.f, 1.f, 1.f, 1.f};
        uint32_t offset = static_cast<uint32_t>(m_TexturedBatchVertices.size());

        m_TexturedBatchVertices.push_back(
            {-1.f, -1.f, 0.f, 1.f, white.r, white.g, white.b, white.a});
        m_TexturedBatchVertices.push_back(
            {1.f, -1.f, 1.f, 1.f, white.r, white.g, white.b, white.a});
        m_TexturedBatchVertices.push_back({1.f, 1.f, 1.f, 0.f, white.r, white.g, white.b, white.a});
        m_TexturedBatchVertices.push_back(
            {-1.f, -1.f, 0.f, 1.f, white.r, white.g, white.b, white.a});
        m_TexturedBatchVertices.push_back({1.f, 1.f, 1.f, 0.f, white.r, white.g, white.b, white.a});
        m_TexturedBatchVertices.push_back(
            {-1.f, 1.f, 0.f, 0.f, white.r, white.g, white.b, white.a});

        m_DrawRuns.push_back({BatchKind::Textured, offset, 6, handle});
    }
    void WebGPURenderer::SetCamera(const DrawCmd::SetCamera& cmd)
    {
        m_CamX = cmd.x;
        m_CamY = cmd.y;
        m_CamZoom = cmd.zoom;
    }
    void WebGPURenderer::ResetCamera(const DrawCmd::ResetCamera&)
    {
        m_CamX = 0.0f;
        m_CamY = 0.0f;
        m_CamZoom = 1.0f;
    }

    TextureHandle WebGPURenderer::CreateTextureFromFile(const std::string& path)
    {
        SDL_Surface* raw = IMG_Load(path.c_str());
        if (!raw)
        {
            Log::Warn("WebGPURenderer", "Could not load image data");
            return k_InvalidTexture;
        }

        SDL_Surface* surface = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(raw);
        if (!surface)
        {
            Log::Warn("WebGPURenderer", "Could convert image into surface");
            return k_InvalidTexture;
        }

        WGPUTextureDescriptor texDesc{};
        texDesc.dimension = WGPUTextureDimension_2D;
        texDesc.size = {static_cast<uint32_t>(surface->w), static_cast<uint32_t>(surface->h), 1};
        texDesc.format = WGPUTextureFormat_RGBA8Unorm;
        texDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount = 1;
        WGPUTexture texture = wgpuDeviceCreateTexture(m_Device, &texDesc);

        WGPUTexelCopyTextureInfo dst{};
        dst.texture = texture;
        dst.mipLevel = 0;
        dst.origin = {0, 0, 0};
        dst.aspect = WGPUTextureAspect_All;

        WGPUTexelCopyBufferLayout layout{};
        layout.offset = 0;
        layout.bytesPerRow = static_cast<uint32_t>(surface->pitch);
        layout.rowsPerImage = static_cast<uint32_t>(surface->h);

        WGPUExtent3D extent{
            static_cast<uint32_t>(surface->w), static_cast<uint32_t>(surface->h), 1};
        wgpuQueueWriteTexture(m_Queue,
                              &dst,
                              surface->pixels,
                              static_cast<size_t>(surface->pitch) * surface->h,
                              &layout,
                              &extent);

        WGPUTextureViewDescriptor viewDesc{};
        viewDesc.format = WGPUTextureFormat_RGBA8Unorm;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.mipLevelCount = 1;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;
        WGPUTextureView view = wgpuTextureCreateView(texture, &viewDesc);

        WGPUBindGroupEntry bgEntries[2] = {};
        bgEntries[0].binding = 0;
        bgEntries[0].sampler = m_DefaultSampler;
        bgEntries[1].binding = 1;
        bgEntries[1].textureView = view;

        WGPUBindGroupDescriptor bgDesc{};
        bgDesc.layout = m_TextureBindGroupLayout;
        bgDesc.entryCount = 2;
        bgDesc.entries = bgEntries;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_Device, &bgDesc);

        TextureHandle handle = m_NextHandle++;
        m_Textures[handle] = {texture, view, bindGroup, {surface->w, surface->h}};

        SDL_DestroySurface(surface);
        return handle;
    }
    TextureDesc WebGPURenderer::GetTextureDesc(TextureHandle handle) const
    {
        auto it = m_Textures.find(handle);
        return it != m_Textures.end() ? it->second.desc : TextureDesc{};
    }
    void WebGPURenderer::DestroyTexture(TextureHandle handle)
    {
        auto it = m_Textures.find(handle);
        if (it == m_Textures.end())
            return;
        wgpuBindGroupRelease(it->second.bindGroup);
        wgpuTextureViewRelease(it->second.view);
        wgpuTextureRelease(it->second.texture);
        m_Textures.erase(it);
    }

    void* WebGPURenderer::GetNativeTextureHandle(TextureHandle handle) const
    {
        // Per renderer.hpp's doc comment: WebGPU ImTextureID is a WGPUTextureView.
        auto it = m_Textures.find(handle);
        return it != m_Textures.end() ? reinterpret_cast<void*>(it->second.view) : nullptr;
    }

    void WebGPURenderer::Resize(int /*width*/, int /*height*/)
    {
        int pixelW = 0, pixelH = 0;
        SDL_GetWindowSizeInPixels(m_Window, &pixelW, &pixelH);
        if (pixelW <= 0 || pixelH <= 0)
            return;

        m_Width = pixelW;
        m_Height = pixelH;

        // Device/surface might not exist yet if this fires before the async
        // adapter/device request completes - in that case OnDeviceReady's
        // first ConfigureSurface() call will pick up the corrected size.
        if (m_Device && m_Surface)
            ConfigureSurface();
    }
    void WebGPURenderer::SetViewportRenderTarget(WGPUTextureView view, int width, int height)
    {
        m_ViewportRenderTargetView = view;
        m_ViewportWidth = width;
        m_ViewportHeight = height;
    }
    void WebGPURenderer::CreateFlatColorPipeline()
    {
        WGPUShaderSourceWGSL wgslDesc{};
        wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgslDesc.code = {kFlatColorShaderWGSL, WGPU_STRLEN};

        WGPUShaderModuleDescriptor shaderDesc{};
        shaderDesc.nextInChain = &wgslDesc.chain;
        WGPUShaderModule shader = wgpuDeviceCreateShaderModule(m_Device, &shaderDesc);

        WGPUVertexAttribute attrs[2] = {
            {.format = WGPUVertexFormat_Float32x2, .offset = 0, .shaderLocation = 0},
            {.format = WGPUVertexFormat_Float32x4,
             .offset = sizeof(float) * 2,
             .shaderLocation = 1},
        };
        WGPUVertexBufferLayout vbLayout{};
        vbLayout.arrayStride = sizeof(FlatVertex);
        vbLayout.stepMode = WGPUVertexStepMode_Vertex;
        vbLayout.attributeCount = 2;
        vbLayout.attributes = attrs;

        WGPUBlendState blend{};
        blend.color = {
            WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha};
        blend.alpha = {
            WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha};

        WGPUColorTargetState colorTarget{};
        colorTarget.format = m_SurfaceFormat;
        colorTarget.blend = &blend;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragState{};
        fragState.module = shader;
        fragState.entryPoint = {"fs_main", WGPU_STRLEN};
        fragState.targetCount = 1;
        fragState.targets = &colorTarget;

        WGPURenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.layout = nullptr;
        pipelineDesc.vertex.module = shader;
        pipelineDesc.vertex.entryPoint = {"vs_main", WGPU_STRLEN};
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vbLayout;
        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDesc.fragment = &fragState;
        pipelineDesc.multisample.count = 1;
        pipelineDesc.multisample.mask = 0xFFFFFFFF;

        m_FlatColorPipeline = wgpuDeviceCreateRenderPipeline(m_Device, &pipelineDesc);
        wgpuShaderModuleRelease(shader);
    }

    void WebGPURenderer::CreateTexturedPipeline()
    {
        WGPUShaderSourceWGSL wgslDesc{};
        wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgslDesc.code = {kTexturedShaderWGSL, WGPU_STRLEN};

        WGPUShaderModuleDescriptor shaderDesc{};
        shaderDesc.nextInChain = &wgslDesc.chain;
        WGPUShaderModule shader = wgpuDeviceCreateShaderModule(m_Device, &shaderDesc);

        WGPUBindGroupLayoutEntry bglEntries[2] = {};
        bglEntries[0].binding = 0;
        bglEntries[0].visibility = WGPUShaderStage_Fragment;
        bglEntries[0].sampler.type = WGPUSamplerBindingType_Filtering;

        bglEntries[1].binding = 1;
        bglEntries[1].visibility = WGPUShaderStage_Fragment;
        bglEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
        bglEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;

        WGPUBindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = 2;
        bglDesc.entries = bglEntries;
        m_TextureBindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_Device, &bglDesc);

        WGPUPipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = &m_TextureBindGroupLayout;
        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(m_Device, &plDesc);

        WGPUVertexAttribute attrs[3] = {
            {.format = WGPUVertexFormat_Float32x2, .offset = 0, .shaderLocation = 0},
            {.format = WGPUVertexFormat_Float32x2,
             .offset = sizeof(float) * 2,
             .shaderLocation = 1},
            {.format = WGPUVertexFormat_Float32x4,
             .offset = sizeof(float) * 4,
             .shaderLocation = 2},
        };
        WGPUVertexBufferLayout vbLayout{};
        vbLayout.arrayStride = sizeof(TexturedVertex);
        vbLayout.stepMode = WGPUVertexStepMode_Vertex;
        vbLayout.attributeCount = 3;
        vbLayout.attributes = attrs;

        WGPUBlendState blend{};
        blend.color = {
            WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha};
        blend.alpha = {
            WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha};

        WGPUColorTargetState colorTarget{};
        colorTarget.format = m_SurfaceFormat;
        colorTarget.blend = &blend;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragState{};
        fragState.module = shader;
        fragState.entryPoint = {"fs_main", WGPU_STRLEN};
        fragState.targetCount = 1;
        fragState.targets = &colorTarget;

        WGPURenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.layout = pipelineLayout;
        pipelineDesc.vertex.module = shader;
        pipelineDesc.vertex.entryPoint = {"vs_main", WGPU_STRLEN};
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vbLayout;
        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDesc.fragment = &fragState;
        pipelineDesc.multisample.count = 1;
        pipelineDesc.multisample.mask = 0xFFFFFFFF;

        m_TexturePipeline = wgpuDeviceCreateRenderPipeline(m_Device, &pipelineDesc);

        wgpuPipelineLayoutRelease(pipelineLayout);
        wgpuShaderModuleRelease(shader);

        WGPUSamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.minFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        samplerDesc.maxAnisotropy = 1;
        m_DefaultSampler = wgpuDeviceCreateSampler(m_Device, &samplerDesc);
    }

    void WebGPURenderer::ToScreen(float wx, float wy, float& sx, float& sy) const
    {
        sx = (wx - m_CamX) * m_CamZoom + CurrentTargetWidth() * 0.5f;
        sy = (wy - m_CamY) * m_CamZoom + CurrentTargetHeight() * 0.5f;
    }
    void WebGPURenderer::PushVertex(float sx, float sy, const Color& col)
    {
        float ndcX = (sx / static_cast<float>(CurrentTargetWidth())) * 2.0f - 1.0f;
        float ndcY = 1.0f - (sy / static_cast<float>(CurrentTargetHeight())) * 2.0f;
        m_BatchVertices.push_back({ndcX, ndcY, col.r, col.g, col.b, col.a});
    }
    void WebGPURenderer::PushTexturedVertex(float sx, float sy, float u, float v, const Color& col)
    {
        float ndcX = (sx / static_cast<float>(CurrentTargetWidth())) * 2.0f - 1.0f;
        float ndcY = 1.0f - (sy / static_cast<float>(CurrentTargetHeight())) * 2.0f;
        m_TexturedBatchVertices.push_back({ndcX, ndcY, u, v, col.r, col.g, col.b, col.a});
    }

    void WebGPURenderer::AppendQuad(float ax,
                                    float ay,
                                    float bx,
                                    float by,
                                    float cx,
                                    float cy,
                                    float dx,
                                    float dy,
                                    const Color& col)
    {
        PushVertex(ax, ay, col);
        PushVertex(bx, by, col);
        PushVertex(cx, cy, col);
        PushVertex(ax, ay, col);
        PushVertex(cx, cy, col);
        PushVertex(dx, dy, col);
    }
    void WebGPURenderer::PushTexturedQuad(float ax,
                                          float ay,
                                          float au,
                                          float av,
                                          float bx,
                                          float by,
                                          float bu,
                                          float bv,
                                          float cx,
                                          float cy,
                                          float cu,
                                          float cv,
                                          float dx,
                                          float dy,
                                          float du,
                                          float dv,
                                          const Color& col)
    {
        PushTexturedVertex(ax, ay, au, av, col);
        PushTexturedVertex(bx, by, bu, bv, col);
        PushTexturedVertex(cx, cy, cu, cv, col);
        PushTexturedVertex(ax, ay, au, av, col);
        PushTexturedVertex(cx, cy, cu, cv, col);
        PushTexturedVertex(dx, dy, du, dv, col);
    }
    void WebGPURenderer::AppendThickLine(
        float x1, float y1, float x2, float y2, const Color& col, float thickness)
    {
        float dx = x2 - x1, dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-5f)
            return;
        float nx = -dy / len * thickness * 0.5f;
        float ny = dx / len * thickness * 0.5f;
        AppendQuad(x1 + nx, y1 + ny, x2 + nx, y2 + ny, x2 - nx, y2 - ny, x1 - nx, y1 - ny, col);
    }

    void WebGPURenderer::EnsureVertexBufferCapacity(size_t requiredBytes)
    {
        if (requiredBytes <= m_VertexBufferCapacityBytes)
            return;

        if (m_VertexBuffer)
            wgpuBufferRelease(m_VertexBuffer);

        size_t newCapacity = std::max(requiredBytes, m_VertexBufferCapacityBytes * 2);
        WGPUBufferDescriptor desc{};
        desc.size = newCapacity;
        desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        m_VertexBuffer = wgpuDeviceCreateBuffer(m_Device, &desc);
        m_VertexBufferCapacityBytes = newCapacity;
    }
    void WebGPURenderer::EnsureTexturedVertexBufferCapacity(size_t requiredBytes)
    {
        if (requiredBytes <= m_TexturedVertexBufferCapacityBytes)
            return;
        if (m_TexturedVertexBuffer)
            wgpuBufferRelease(m_TexturedVertexBuffer);

        size_t newCapacity = std::max(requiredBytes, m_TexturedVertexBufferCapacityBytes * 2);
        WGPUBufferDescriptor desc{};
        desc.size = newCapacity;
        desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        m_TexturedVertexBuffer = wgpuDeviceCreateBuffer(m_Device, &desc);
        m_TexturedVertexBufferCapacityBytes = newCapacity;
    }

    void WebGPURenderer::BeginFlatRun()
    {
        m_FlatRunStart = m_BatchVertices.size();
    }

    void WebGPURenderer::EndFlatRun()
    {
        size_t count = m_BatchVertices.size() - m_FlatRunStart;
        if (count == 0)
            return;

        if (!m_DrawRuns.empty() && m_DrawRuns.back().kind == BatchKind::Flat)
            m_DrawRuns.back().vertexCount += static_cast<uint32_t>(count);
        else
            m_DrawRuns.push_back({BatchKind::Flat,
                                  static_cast<uint32_t>(m_FlatRunStart),
                                  static_cast<uint32_t>(count)});
    }
    void WebGPURenderer::FlushDrawRuns(WGPURenderPassEncoder pass)
    {
        if (m_DrawRuns.empty())
            return;

        if (!m_BatchVertices.empty())
        {
            size_t byteSize = m_BatchVertices.size() * sizeof(FlatVertex);
            EnsureVertexBufferCapacity(byteSize);
            wgpuQueueWriteBuffer(m_Queue, m_VertexBuffer, 0, m_BatchVertices.data(), byteSize);
        }
        if (!m_TexturedBatchVertices.empty())
        {
            size_t byteSize = m_TexturedBatchVertices.size() * sizeof(TexturedVertex);
            EnsureTexturedVertexBufferCapacity(byteSize);
            wgpuQueueWriteBuffer(
                m_Queue, m_TexturedVertexBuffer, 0, m_TexturedBatchVertices.data(), byteSize);
        }

        bool anyBound = false;
        BatchKind boundKind{};

        for (const DrawRun& run : m_DrawRuns)
        {
            if (run.kind == BatchKind::Flat)
            {
                if (!anyBound || boundKind != BatchKind::Flat)
                {
                    wgpuRenderPassEncoderSetPipeline(pass, m_FlatColorPipeline);
                    wgpuRenderPassEncoderSetVertexBuffer(
                        pass, 0, m_VertexBuffer, 0, m_BatchVertices.size() * sizeof(FlatVertex));
                    boundKind = BatchKind::Flat;
                    anyBound = true;
                }
                wgpuRenderPassEncoderDraw(pass, run.vertexCount, 1, run.vertexOffset, 0);
            }
            else
            {
                auto it = m_Textures.find(run.textureHandle);
                if (it == m_Textures.end())
                    continue; // destroyed mid-frame: drop its run rather than crash

                if (!anyBound || boundKind != BatchKind::Textured)
                {
                    wgpuRenderPassEncoderSetPipeline(pass, m_TexturePipeline);
                    wgpuRenderPassEncoderSetVertexBuffer(pass,
                                                         0,
                                                         m_TexturedVertexBuffer,
                                                         0,
                                                         m_TexturedBatchVertices.size() *
                                                             sizeof(TexturedVertex));
                    boundKind = BatchKind::Textured;
                    anyBound = true;
                }
                // Bind group changes on every textured run regardless of
                // boundKind.
                //  a texture change always needs a new bind group
                // even when the previous run was also Textured.
                wgpuRenderPassEncoderSetBindGroup(pass, 0, it->second.bindGroup, 0, nullptr);
                wgpuRenderPassEncoderDraw(pass, run.vertexCount, 1, run.vertexOffset, 0);
            }
        }

        m_BatchVertices.clear();
        m_TexturedBatchVertices.clear();
        m_DrawRuns.clear();
    }
} // namespace Cadmium
