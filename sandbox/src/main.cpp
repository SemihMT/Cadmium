#include <cadmium/core/Engine.hpp>
#include <cadmium/core/Application.hpp>
#include <iostream>

class SandboxApp : public Cadmium::Application
{
public:
  void OnStart() override
  {
    // setup goes here
    std::cout << "SandboxApp OnStart\n";
  }

  void OnUpdate(float dt) override
  {
    // game logic goes here
  }

  void OnRender() override
  {
    // drawing goes here
  }

  void OnEvent(SDL_Event &event) override
  {
    if (event.type == SDL_EVENT_KEY_DOWN)
      if (event.key.key == SDLK_ESCAPE)
        Quit();
  }
};

int main()
{
  try
  {
    Cadmium::Engine engine(
        std::make_unique<SandboxApp>(),
        "Cadmium Sandbox",
        1280, 720);
    engine.Run();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
