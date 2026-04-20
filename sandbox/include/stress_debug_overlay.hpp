#pragma once

#include <cadmium/core/layer.hpp>
#include "menu_events.hpp"
#include <SDL3/SDL.h>
#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Sandbox
{
  class StressDebugOverlay : public Cadmium::Layer
  {
  public:
    StressDebugOverlay() : Cadmium::Layer("StressDebug") {}

    void OnAttach() override;
    void OnImGuiRender() override;

  private:
    void SpawnBatch(int count);
    void ClearAll();

    int   m_BatchSize{1000};
    float m_Speed{200.0f};
  };

} // namespace Sandbox
