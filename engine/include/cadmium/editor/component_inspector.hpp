#ifndef CADMIUM_EDITOR_COMPONENT_INSPECTOR
#define CADMIUM_EDITOR_COMPONENT_INSPECTOR
#include <cadmium/ecs/world.hpp>
#include <functional>
#include <vector>
#include <string>

namespace Cadmium::Editor
{
  struct ComponentInspectorEntry
  {
    std::string name;
    std::function<bool(World&, Entity)> has;
    std::function<void(World&, Entity)> draw;
  };

  class ComponentInspector
  {
  public:
    static ComponentInspector& Get() { static ComponentInspector i; return i; }

    template<typename T>
    void Register(std::string name, std::function<void(T&)> draw)
    {
      m_Entries.push_back({
        std::move(name),
        [](World& w, Entity e) { return w.HasComponent<T>(e); },
        [draw](World& w, Entity e) { draw(w.GetComponent<T>(e)); }
      });
    }

    const std::vector<ComponentInspectorEntry>& Entries() const { return m_Entries; }

  private:
    std::vector<ComponentInspectorEntry> m_Entries;
  };
} // namespace Cadmium::Editor
#endif // CADMIUM_EDITOR_COMPONENT_INSPECTOR
