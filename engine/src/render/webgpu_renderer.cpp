#include <cadmium/render/native_surface.hpp>
#include <webgpu/webgpu.h>
#include <cadmium/core/logger.hpp>
#include <cadmium/render/webgpu_renderer.hpp>

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
#include <windows.h>
#endif

#include <SDL3/SDL.h>

namespace Cadmium
{
    WebGPURenderer::WebGPURenderer(SDL_Window* window, int width, int height) : m_Window{window}, m_Width(width), m_Height(height)
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
        // (wgpuDeviceRelease in ~WebGPURenderer) - that one's expected, not an error.
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

    void WebGPURenderer::BeginFrame()
    {
        wgpuSurfaceGetCurrentTexture(m_Surface, &m_CurrentSurfaceTexture);
    }

    void WebGPURenderer::EndFrame()
    {
        WGPUTextureViewDescriptor viewDesc{};
        viewDesc.format = m_SurfaceFormat;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;

        // Step 1 success criterion: clear the surface to a visible color
        // every frame, no batching/pipelines yet - those are step 2.
        WGPUTextureView view = wgpuTextureCreateView(m_CurrentSurfaceTexture.texture, &viewDesc);


        WGPURenderPassColorAttachment colorAttachment{};
        colorAttachment.view = view;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.5, 0.05, 0.15, 1.0};
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_Device, nullptr);
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);
        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(m_Queue, 1, &cmd);

        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(view);
        #ifndef CADMIUM_PLATFORM_WEB
        wgpuSurfacePresent(m_Surface);
        #endif
        // wgpuSurfacePresent isn't called explicitly on Emscripten -
        // presentation happens automatically at the end of the browser's
        // rAF-driven frame when using WGPUPresentMode_Fifo.
    }
} // namespace Cadmium
