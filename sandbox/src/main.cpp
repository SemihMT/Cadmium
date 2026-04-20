#include <cadmium/core/engine.hpp>
#include <iostream>
#include <memory>
#include "menu_scene.hpp"


int main()
{
  try
  {
    Cadmium::Engine engine("Cadmium — Asteroids", 1280, 720);
    engine.DisableDefaultBackground();
    engine.SetTargetFPS(60);
    engine.PushScene(std::make_unique<Sandbox::MenuScene>());
    engine.Run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
