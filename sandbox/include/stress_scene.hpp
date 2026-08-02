#ifndef SANDBOX_STRESS_SCENE
#define SANDBOX_STRESS_SCENE

#include <cadmium/core/scene.hpp>

namespace Sandbox
{
    class MenuScene;

    class StressScene : public Cadmium::Scene
    {
      public:
        StressScene() : Cadmium::Scene("StressTest") {}
        void OnEnter() override;
        void OnDestroy() override;

      private:
        Cadmium::SubscriptionToken m_ReturnToken;
    };

} // namespace Sandbox
#endif // SANDBOX_STRESS_SCENE
