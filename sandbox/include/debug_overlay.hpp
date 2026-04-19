#ifndef SANDBOX_DEBUG_OVERLAY_HPP
#define SANDBOX_DEBUG_OVERLAY_HPP

#include <cadmium/core/layer.hpp>
#include "game_state.hpp"
#include "events.hpp"
#include <memory>
#include <SDL3/SDL.h>
#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Sandbox
{
  class DebugOverlay : public Cadmium::Layer
  {
  public:
    explicit DebugOverlay(std::shared_ptr<GameState> state)
      : Cadmium::Layer("Debug")
      , m_State{std::move(state)}
    {}

    void OnImGuiRender() override;

  private:
    std::shared_ptr<GameState> m_State;
  };

} // namespace Sandbox

#endif // SANDBOX_DEBUG_OVERLAY_HPP
