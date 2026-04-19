#include <cadmium/core/engine.hpp>
#include "game_state.hpp"
#include "world_layer.hpp"
#include "render_layer.hpp"
#include "hud_layer.hpp"
#include "debug_overlay.hpp"
#include <iostream>
#include <memory>

int main()
{
  try
  {
    auto state = std::make_shared<Sandbox::GameState>();

    Cadmium::Engine engine("Cadmium — Asteroids", 1280, 720);
    engine.DisableDefaultBackground();
    engine.SetTargetFPS(60);

    engine.PushLayer(std::make_unique<Sandbox::WorldLayer>(state));
    engine.PushLayer(std::make_unique<Sandbox::RenderLayer>(state));
    engine.PushLayer(std::make_unique<Sandbox::HUDLayer>(state));
    engine.PushOverlay(std::make_unique<Sandbox::DebugOverlay>(state));

    engine.Run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
