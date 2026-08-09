#include "cadmium/core/draw_command_queue.hpp"
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
}

namespace Cadmium
{
    WebGPURenderer::WebGPURenderer(SDL_Window* window, int width, int height)
        : m_Window{window}, m_Width(width), m_Height(height)
    {
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
        FlushBatch(pass); // the game's flat-color batch belongs here
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
    WGPUTextureView surfaceView = wgpuTextureCreateView(m_CurrentSurfaceTexture.texture, &viewDesc);

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
        FlushBatch(pass2); // no viewport this frame - the batch belongs in the only pass there is
    InvokeImGuiHook(pass2); // ImGui always fires here - the pass that actually reaches the screen
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
        float x1, y1, x2, y2;
        ToScreen(l.x1, l.y1, x1, y1);
        ToScreen(l.x2, l.y2, x2, y2);
        AppendThickLine(x1, y1, x2, y2, l.color, k_LineThicknessPx);
    }

    void WebGPURenderer::DrawRect(const DrawCmd::Rect& r)
    {
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
    }

    void WebGPURenderer::DrawCircle(const DrawCmd::Circle& c)
    {
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
    }

    void WebGPURenderer::DrawPolygon(const DrawCmd::Polygon& p)
    {
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
    }
    void WebGPURenderer::DrawText(const DrawCmd::Text&) {}
    void WebGPURenderer::DrawSprite(const DrawCmd::Sprite&) {}
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
    void WebGPURenderer::DrawFullscreenTexture(TextureHandle) {}

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

    void WebGPURenderer::FlushBatch(WGPURenderPassEncoder pass)
    {
        if (m_BatchVertices.empty())
            return;

        size_t byteSize = m_BatchVertices.size() * sizeof(FlatVertex);
        EnsureVertexBufferCapacity(byteSize);
        wgpuQueueWriteBuffer(m_Queue, m_VertexBuffer, 0, m_BatchVertices.data(), byteSize);

        wgpuRenderPassEncoderSetPipeline(pass, m_FlatColorPipeline);
        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_VertexBuffer, 0, byteSize);
        wgpuRenderPassEncoderDraw(pass, static_cast<uint32_t>(m_BatchVertices.size()), 1, 0, 0);

        m_BatchVertices.clear();
    }

} // namespace Cadmium
