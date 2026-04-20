#include <cadmium/core/engine.hpp>
#include "game_state.hpp"
#include "world_layer.hpp"
#include "render_layer.hpp"
#include "hud_layer.hpp"
#include "debug_overlay.hpp"
#include <iostream>
#include <memory>

class AsteroidsScene : public Cadmium::Scene
{
public:
  AsteroidsScene() : Scene("Asteroids") {}

  void OnEnter() override
  {
    auto state = std::make_shared<Sandbox::GameState>();
    PushLayer(std::make_unique<Sandbox::WorldLayer>(state));
    PushLayer(std::make_unique<Sandbox::RenderLayer>(state));
    PushLayer(std::make_unique<Sandbox::HUDLayer>(state));
    PushOverlay(std::make_unique<Sandbox::DebugOverlay>(state));
  }
};

int main()
{
  try
  {
    Cadmium::Engine engine("Cadmium — Asteroids", 1280, 720);
    engine.DisableDefaultBackground();
    engine.SetTargetFPS(60);
    engine.PushScene(std::make_unique<AsteroidsScene>());
    engine.Run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
