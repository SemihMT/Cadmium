#include <cadmium/core/layer.hpp>
#include <cadmium/core/scene.hpp>
#include <cadmium/ecs/world.hpp>
#include <stdexcept>

namespace Cadmium
{
  World &Layer::GetWorld()
  {
    Scene *scene = m_Context->GetActiveScene();
    if (!scene)
      throw std::runtime_error("GetWorld called with no active scene");
    return scene->GetWorld();
  }
  void Layer::PushScene(std::unique_ptr<Scene> scene) { m_Context->PushScene(std::move(scene)); }
  void Layer::PopScene() { m_Context->PopScene(); }
  void Layer::ReplaceScene(std::unique_ptr<Scene> scene) { m_Context->ReplaceScene(std::move(scene)); }

}
