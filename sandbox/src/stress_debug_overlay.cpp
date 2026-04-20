#include "stress_debug_overlay.hpp"
#include "components.hpp"
#include <cadmium/ecs/world.hpp>
#include <cmath>
#include <random>

namespace Sandbox
{
  static std::mt19937                          s_Rng{std::random_device{}()};
  static std::uniform_real_distribution<float> s_Dist{0.0f, 1.0f};

  static float RandomFloat(float min, float max)
  {
    return min + s_Dist(s_Rng) * (max - min);
  }

  void StressDebugOverlay::OnAttach() {}

  void StressDebugOverlay::SpawnBatch(int count)
  {
    auto& world  = GetWorld();
    float width  = static_cast<float>(GetWidth());
    float height = static_cast<float>(GetHeight());

    for (int i = 0; i < count; i++)
    {
      auto entity = world.CreateEntity();

      float angle = RandomFloat(0.0f, 2.0f * 3.14159f);

      world.AddComponent<Cadmium::Transform>(entity, {
        .x        = RandomFloat(0.0f, width),
        .y        = RandomFloat(0.0f, height),
        .rotation = 0.0f
      });

      world.AddComponent<Cadmium::Velocity>(entity, {
        .x = std::cos(angle) * m_Speed,
        .y = std::sin(angle) * m_Speed
      });

      world.AddComponent<Sandbox::StressTag>(entity, {});
    }
  }

  void StressDebugOverlay::ClearAll()
  {
    auto& world    = GetWorld();
    auto  entities = world.QueryEntities<Sandbox::StressTag>();
    for (auto entity : entities)
      world.DestroyEntity(entity);
  }

  void StressDebugOverlay::OnImGuiRender()
  {
#ifdef CADMIUM_IMGUI
    ImGui::Begin("ECS Stress Test");

    ImGui::SeparatorText("Performance");
    ImGui::Text("FPS:      %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Entities: %zu",  GetWorld().EntityCount());

    ImGui::SeparatorText("Spawn");
    ImGui::SliderInt("Batch Size", &m_BatchSize, 100, 10000);
    ImGui::SliderFloat("Speed",    &m_Speed,     0.0f, 1000.0f);

    if (ImGui::Button("Spawn Batch"))
      SpawnBatch(m_BatchSize);

    ImGui::SameLine();

    if (ImGui::Button("Spawn 10x"))
      for (int i = 0; i < 10; i++)
        SpawnBatch(m_BatchSize);

    ImGui::SameLine();

    if (ImGui::Button("Clear All"))
      ClearAll();

    ImGui::SeparatorText("Navigation");
    if (ImGui::Button("Return to Menu"))
      Post(ReturnToMenuEvent{});

    ImGui::End();
#endif
  }

} // namespace Sandbox
