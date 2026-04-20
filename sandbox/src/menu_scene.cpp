#include "menu_scene.hpp"
#include "menu_layer.hpp"

namespace Sandbox
{
  void MenuScene::OnEnter()
  {
    SetDefaultBackground(false);
    PushOverlay(std::make_unique<MenuLayer>());
  }

} // namespace Sandbox
