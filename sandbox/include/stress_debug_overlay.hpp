#ifndef SANDBOX_STRESS_DEBUG_OVERLAY
#define SANDBOX_STRESS_DEBUG_OVERLAY

#include <SDL3/SDL.h>
#include <cadmium/core/layer.hpp>

#ifdef CADMIUM_IMGUI
#include <cadmium/editor/asset_panel.hpp>
#include <imgui.h>

#endif

namespace Sandbox
{
    class StressDebugOverlay : public Cadmium::Layer
    {
      public:
        StressDebugOverlay() : Cadmium::Layer("StressDebug") {}

        void OnImGuiRender() override;

      private:
        void SpawnBatch(int count);
        void ClearAll();

        int m_BatchSize{1000};
        float m_Speed{200.0f};
    };

} // namespace Sandbox
#endif // SANDBOX_STRESS_DEBUG_OVERLAY
