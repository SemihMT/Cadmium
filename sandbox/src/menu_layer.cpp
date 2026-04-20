#include "menu_layer.hpp"
#include "asteroids_scene.hpp"
#include "stress_scene.hpp"

namespace Sandbox
{
  void MenuLayer::OnEvent(SDL_Event& event)
  {
    if (event.type == SDL_EVENT_KEY_DOWN)
      if (event.key.key == SDLK_ESCAPE)
        Quit();
  }

  void MenuLayer::OnImGuiRender()
  {
#ifdef CADMIUM_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    float    w  = io.DisplaySize.x;
    float    h  = io.DisplaySize.y;

    // Fullscreen window, no decoration
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({w, h});
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("##menu",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration  |
                 ImGuiWindowFlags_NoMove        |
                 ImGuiWindowFlags_NoSavedSettings);

    // Center content
    float buttonW = 300.0f;
    float buttonH = 60.0f;
    ImGui::SetCursorPos({ (w - buttonW) * 0.5f, h * 0.35f });

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.2f, 0.4f, 0.8f, 1.0f));

    ImGui::Text("CADMIUM ENGINE");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetCursorPosX((w - buttonW) * 0.5f);
    if (ImGui::Button("Asteroids", { buttonW, buttonH }))
      ReplaceScene(std::make_unique<AsteroidsScene>());

    ImGui::Spacing();

    ImGui::SetCursorPosX((w - buttonW) * 0.5f);
    if (ImGui::Button("ECS Stress Test", { buttonW, buttonH }))
      ReplaceScene(std::make_unique<StressScene>());

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SetCursorPosX((w - buttonW) * 0.5f);
    if (ImGui::Button("Quit", { buttonW, buttonH }))
      Quit();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::SetCursorPos({ 10.0f, h - 25.0f });
    ImGui::TextDisabled("FPS: %.1f", io.Framerate);

    ImGui::End();
#endif
  }

} // namespace Sandbox
