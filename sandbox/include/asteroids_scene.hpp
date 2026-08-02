#ifndef SANDBOX_ASTEROIDS_SCENE
#define SANDBOX_ASTEROIDS_SCENE

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
  };

} // namespace Sandbox
#endif // SANDBOX_ASTEROIDS_SCENE
