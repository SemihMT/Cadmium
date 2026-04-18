#include <cadmium/core/engine.hpp>
#include <cadmium/core/application.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

#include <iostream>

class SandboxApp : public Cadmium::Application
{
public:
  void OnStart() override
  {
  }

  void OnUpdate(float dt) override
  {
    m_DeltaTime = dt;
  }

  void OnRender() override
  {
  }

  void OnEvent(SDL_Event &event) override
  {
    if (event.type == SDL_EVENT_KEY_DOWN)
      if (event.key.key == SDLK_ESCAPE)
        Quit();
  }

#ifdef CADMIUM_IMGUI
  void OnImGuiRender() override
  {
    ImGui::Begin("Debug");
    ImGui::Text("Delta time: %.4f", m_DeltaTime);
    ImGui::Text("FPS: %.1f", 1.0f / m_DeltaTime);
    ImGui::End();
  }
#endif

private:
  float m_DeltaTime{0.0f};
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
