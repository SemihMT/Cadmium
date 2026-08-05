#include <cadmium/core/engine.hpp>
#include <iostream>
#include <memory>
#include <cadmium/core/scene.hpp>
#include <cadmium/render/renderer_backend.hpp>
#include <cadmium/editor/editor_overlay_layer.hpp>


int main()
{
  try
  {
    Cadmium::Engine engine("Cadmium - Asteroids", 1280, 720, Cadmium::RendererBackend::WebGPU);
    //engine.PushGlobalOverlay(std::make_unique<Cadmium::Editor::EditorOverlayLayer>(engine.GetAssets(), engine.GetRenderer(),&engine));
    engine.DisableDefaultBackground();
    engine.SetTargetFPS(60);
    engine.PushScene(std::make_unique<Cadmium::Scene>("Test"));
    engine.Run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
