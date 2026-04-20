#include "debug_overlay.hpp"
#include "menu_events.hpp"
#include <cadmium/ecs/world.hpp>

namespace Sandbox
{
  void DebugOverlay::OnImGuiRender()
  {
#ifdef CADMIUM_IMGUI
    ImGui::Begin("Debug");

    ImGui::SeparatorText("Performance");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::SeparatorText("Game State");
    ImGui::Text("Score:     %d", m_State->score);
    ImGui::Text("Wave:      %d", m_State->wave);
    ImGui::Text("Asteroids: %zu", m_State->asteroidCount);
    ImGui::Text("Debris entities: %zu", GetWorld().EntityCount());
    ImGui::Text("Bullets:   %zu", m_State->bulletCount);
    ImGui::Text("Game Over: %s", m_State->gameOver ? "yes" : "no");
    ImGui::Text("Ship Alive: %s", m_State->ship.alive ? "yes" : "no");
    ImGui::Text("Ship Pos:  (%.1f, %.1f)",
                m_State->ship.x, m_State->ship.y);
    ImGui::Text("Ship Vel:  (%.1f, %.1f)",
                m_State->ship.vx, m_State->ship.vy);

    ImGui::SeparatorText("Actions");

    if (ImGui::Button("Spawn Asteroid"))
      Post(SpawnAsteroidEvent{});

    ImGui::SameLine();

    if (ImGui::Button("Toggle Invincibility"))
      Post(ToggleInvincibilityEvent{});

    ImGui::SameLine();

    if (ImGui::Button("Restart"))
      Post(RestartEvent{});

    ImGui::SeparatorText("Pools");
    ImGui::Text("Asteroid pool: %zu / %zu",
                m_State->asteroidCount,
                GameState::k_MaxAsteroids);
    ImGui::Text("Bullet pool:   %zu / %zu",
                m_State->bulletCount,
                GameState::k_MaxBullets);

    ImGui::SeparatorText("Navigation");
    if (ImGui::Button("Return to Menu"))
      Post(ReturnToMenuEvent{});
    ImGui::End();
#endif
  }

} // namespace Sandbox
