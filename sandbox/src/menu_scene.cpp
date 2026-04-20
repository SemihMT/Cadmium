#include "menu_scene.hpp"
#include "menu_layer.hpp"

namespace Sandbox
{
  void MenuScene::OnEnter()
  {
    PushOverlay(std::make_unique<MenuLayer>());
  }

} // namespace Sandbox
