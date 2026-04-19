#include <cadmium/core/engine.hpp>
#include <cadmium/core/application.hpp>
#include <SDL3/SDL.h>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

#include <iostream>
#include <cmath>

static void DrawFilledCircle(SDL_Renderer *renderer, int cx, int cy, int radius)
{
  for (int y = -radius; y <= radius; y++)
  {
    int dx = static_cast<int>(std::sqrt(radius * radius - y * y));
    SDL_RenderLine(renderer, cx - dx, cy + y, cx + dx, cy + y);
  }
}

class SandboxApp : public Cadmium::Application
{
public:
  void OnStart() override
  {
    Reset();
  }

  void OnFixedUpdate(float dt) override
  {
    m_VelocityY += m_Gravity * dt;

    m_PosX += m_VelocityX * dt;
    m_PosY += m_VelocityY * dt;

    // Bounce off left/right
    if (m_PosX - m_Radius < 0.0f)
    {
      m_PosX = static_cast<float>(m_Radius);
      m_VelocityX = std::abs(m_VelocityX) * m_Damping;
    }
    if (m_PosX + m_Radius > GetWidth())
    {
      m_PosX = static_cast<float>(GetWidth() - m_Radius);
      m_VelocityX = -std::abs(m_VelocityX) * m_Damping;
    }

    // Bounce off top/bottom
    if (m_PosY - m_Radius < 0.0f)
    {
      m_PosY = static_cast<float>(m_Radius);
      m_VelocityY = std::abs(m_VelocityY) * m_Damping;
    }
    if (m_PosY + m_Radius > static_cast<float>(GetHeight()))
    {
      m_PosY = static_cast<float>(GetHeight() - m_Radius);
      m_VelocityY = -std::abs(m_VelocityY) * m_Damping;
    }
  }

  void OnRender(SDL_Renderer *renderer) override
  {
    SDL_SetRenderDrawColor(renderer,
                           static_cast<Uint8>(m_Color[0] * 255),
                           static_cast<Uint8>(m_Color[1] * 255),
                           static_cast<Uint8>(m_Color[2] * 255),
                           255);
    DrawFilledCircle(renderer,
                     static_cast<int>(m_PosX),
                     static_cast<int>(m_PosY),
                     m_Radius);
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
    ImGui::Begin("Ball Controls");

    ImGui::SeparatorText("Physics");
    ImGui::SliderFloat("Gravity", &m_Gravity, 0.0f, 2000.0f);
    ImGui::SliderFloat("Damping", &m_Damping, 0.0f, 1.0f);

    ImGui::SeparatorText("Ball");
    if (ImGui::SliderInt("Radius", &m_Radius, 5, 200))
      m_Radius = std::clamp(m_Radius, 5, std::min(GetWidth(), GetHeight()) / 2);
    ImGui::ColorEdit3("Color", m_Color);

    ImGui::SeparatorText("Actions");
    if (ImGui::Button("Reset"))
      Reset();

    ImGui::Spacing();
    ImGui::SeparatorText("Debug");
    ImGui::Text("Position: (%.1f, %.1f)", m_PosX, m_PosY);
    ImGui::Text("Velocity: (%.1f, %.1f)", m_VelocityX, m_VelocityY);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
  }
#endif

private:
  void Reset()
  {
    m_PosX =  GetWidth() / 2.0f;
    m_PosY = GetHeight() / 2.0f;
    m_VelocityX = 400.0f;
    m_VelocityY = -600.0f;
  }

  // Ball state
  float m_PosX{640.0f};
  float m_PosY{360.0f};
  float m_VelocityX{400.0f};
  float m_VelocityY{-600.0f};
  int m_Radius{30};
  float m_Color[3]{1.0f, 0.4f, 0.1f};

  // Physics parameters
  float m_Gravity{980.0f};
  float m_Damping{0.85f};
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
