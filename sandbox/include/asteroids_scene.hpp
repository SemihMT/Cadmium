#pragma once

#include <cadmium/core/scene.hpp>
#include <sol/sol.hpp>
namespace Sandbox
{
  class MenuScene;

  class AsteroidsScene : public Cadmium::Scene
  {
  public:
    AsteroidsScene() : Cadmium::Scene("Asteroids") {}
    void OnEnter()   override;
    void OnDestroy() override;

  private:
    Cadmium::SubscriptionToken m_ReturnToken;
    // TESTING
    sol::state m_Lua;
  };

} // namespace Sandbox
