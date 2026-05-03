#ifndef CADMIUM_ENTITY_REGISTRY_HPP
#define CADMIUM_ENTITY_REGISTRY_HPP
#include <sol/sol.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <SDL3/SDL.h>
#include <cadmium/ecs/components.hpp>

namespace Cadmium
{

  class EntityRegistry
  {
  public:
    struct Entry
    {
      sol::table table;   // for field lookups, identity comparison, Find/FindAll
      sol::object handle; // EntityHandle userdata - passed as self to hooks
      Entity      cppEntity;
      Registry*   registry;
    };

    void Register(sol::table table, sol::object handle)
    {
      m_Entities.push_back({std::move(table), std::move(handle)});
    }

    void MarkForDestroy(const sol::table &entity)
    {
      for (size_t i = 0; i < m_Entities.size(); ++i)
      {
        if (m_Entities[i].table == entity)
        {
          m_PendingDestroy.push_back(i);
          return;
        }
      }
    }

    void FlushDestroyed()
    {
      if (m_PendingDestroy.empty())
        return;

      std::sort(m_PendingDestroy.begin(), m_PendingDestroy.end(),
                std::greater<size_t>());

      for (size_t idx : m_PendingDestroy)
      {
        if (idx >= m_Entities.size())
          continue;

        Entry &entry = m_Entities[idx];

        sol::protected_function onDetach = entry.table["OnDetach"];
        if (onDetach.valid())
        {
          auto r = onDetach(entry.handle);
          if (!r.valid())
          {
            sol::error err = r;
            SDL_Log("[Entity] OnDetach error: %s", err.what());
          }
        }

        m_Entities.erase(m_Entities.begin() + idx);
      }

      m_PendingDestroy.clear();
    }

    sol::object Find(const std::string &name, sol::state_view lua) const
    {
      for (const auto &entry : m_Entities)
      {
        sol::object n = entry.table["name"];
        if (n.is<std::string>() && n.as<std::string>() == name)
          return entry.handle;
      }
      return sol::nil;
    }

    sol::table FindAll(const std::string &tag, sol::state_view lua) const
    {
      sol::table results = lua.create_table();
      int i = 1;
      for (const auto &entry : m_Entities)
      {
        sol::object t = entry.table["tag"];
        if (t.is<std::string>() && t.as<std::string>() == tag)
          results[i++] = entry.handle;
      }
      return results;
    }

    sol::table FindNear(float x, float y, float radius,
                        sol::state_view lua) const
    {
      sol::table results = lua.create_table();
      float r2 = radius * radius;
      int i = 1;
      for (const auto &entry : m_Entities)
      {
        Transform *t = entry.registry->TryGetComponent<Transform>(entry.cppEntity);
        if (!t)
          continue;

        float dx = t->position.x - x;
        float dy = t->position.y - y;
        if (dx * dx + dy * dy <= r2)
          results[i++] = entry.handle;
      }
      return results;
    }

    int Count() const { return (int)m_Entities.size(); }

    const std::vector<Entry> &All() const { return m_Entities; }

    void Clear()
    {
      for (auto &entry : m_Entities)
      {
        sol::protected_function onDetach = entry.table["OnDetach"];
        if (onDetach.valid())
          onDetach(entry.handle);
      }
      m_Entities.clear();
      m_PendingDestroy.clear();
    }

  private:
    std::vector<Entry> m_Entities;
    std::vector<size_t> m_PendingDestroy;
  };

} // namespace Cadmium
#endif // CADMIUM_ENTITY_REGISTRY_HPP
