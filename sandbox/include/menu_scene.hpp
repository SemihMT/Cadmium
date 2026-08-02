#ifndef SANDBOX_MENU_SCENE
#define SANDBOX_MENU_SCENE

#include <cadmium/core/scene.hpp>

namespace Sandbox
{
    class MenuLayer;
    class MenuScene : public Cadmium::Scene
    {
      public:
        MenuScene() : Cadmium::Scene("Menu") {}
        void OnEnter() override;
    };

} // namespace Sandbox
#endif // SANDBOX_MENU_SCENE
