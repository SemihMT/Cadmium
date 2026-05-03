#ifndef CADMIUM_SCRIPT_UPDATE_LAYER_HPP
#define CADMIUM_SCRIPT_UPDATE_LAYER_HPP
#include <cadmium/core/layer.hpp>
#include <cadmium/scripting/entity_registry.hpp>
#include <sol/sol.hpp>
#include <SDL3/SDL.h>

namespace Cadmium
{
  // Drives the Lua script lifecycle each frame.
  // Calls scene-level hooks (OnUpdate, OnRender, OnFixedUpdate) from the
  // scene environment, and per-entity hooks on all registered entities.
  class ScriptUpdateLayer : public Layer
  {
  public:
    ScriptUpdateLayer(sol::environment &env, EntityRegistry &registry)
        : Layer("ScriptUpdateLayer"), m_Env(env), m_Registry(registry)
    {
    }

    // ── OnUpdate ──────────────────────────────────────────────────────────
    void OnUpdate(float dt) override
    {
      CallEnvHook("OnUpdate", dt);

      for (const auto &entry : m_Registry.All())
      {
        sol::object active = entry.table["active"];
        if (active.is<bool>() && !active.as<bool>())
          continue;

        sol::object hookObj = entry.table["OnUpdate"];
        if (!hookObj.is<sol::protected_function>())
          continue;

        sol::protected_function hook = hookObj;
        auto r = hook(entry.handle, dt);
        if (!r.valid())
        {
          sol::error err = r;
          SDL_Log("[Entity:OnUpdate] error: %s", err.what());
        }
      }
    }

    // ── OnFixedUpdate ─────────────────────────────────────────────────────
    void OnFixedUpdate(float dt) override
    {
      CallEnvHook("OnFixedUpdate", dt);

      for (const auto &entry : m_Registry.All())
      {
        sol::object active = entry.table["active"];
        if (active.is<bool>() && !active.as<bool>())
          continue;

        sol::object hookObj = entry.table["OnFixedUpdate"];
        if (!hookObj.is<sol::protected_function>())
          continue;

        sol::protected_function hook = hookObj;
        auto r = hook(entry.handle, dt);
        if (!r.valid())
        {
          sol::error err = r;
          SDL_Log("[Entity:OnFixedUpdate] error: %s", err.what());
        }
      }
    }

    // ── OnRender ──────────────────────────────────────────────────────────
    // Note: OnRender on entities only pushes to DrawCommandQueue.
    // ScriptRenderLayer drains it. Ordering is guaranteed by layer stack.
    void OnRender(SDL_Renderer *) override
    {
      CallEnvHook("OnRender");

      for (const auto &entry : m_Registry.All())
      {
        sol::object visible = entry.table["visible"];
        if (visible.is<bool>() && !visible.as<bool>())
          continue;

        sol::object active = entry.table["active"];
        if (active.is<bool>() && !active.as<bool>())
          continue;

        sol::object hookObj = entry.table["OnRender"];
        if (!hookObj.is<sol::protected_function>())
          continue;

        sol::protected_function hook = hookObj;
        auto r = hook(entry.handle); // handle as self
        if (!r.valid())
        {
          sol::error err = r;
          SDL_Log("[Entity:OnRender] error: %s", err.what());
        }
      }

      // End of render - safe to remove destroyed entities
      m_Registry.FlushDestroyed();
    }

    // ── OnEvent ───────────────────────────────────────────────────────────
    void OnEvent(SDL_Event &event) override
    {
      sol::table evtTable = BuildEventTable(event);
      if (!evtTable.valid())
        return;

      CallEnvHook("OnEvent");

      for (const auto &entry : m_Registry.All())
      {
        sol::object active = entry.table["active"];
        if (active.is<bool>() && !active.as<bool>())
          continue;

        sol::object hookObj = entry.table["OnEvent"];
        if (!hookObj.is<sol::protected_function>())
          continue;

        sol::protected_function hook = hookObj;
        hook(entry.handle, evtTable); // handle as self
      }
    }

  private:
    sol::environment &m_Env;
    EntityRegistry &m_Registry;

    // ── Scene-level hook helpers ──────────────────────────────────────────

    void CallEnvHook(const std::string &name)
    {
      sol::protected_function f = m_Env[name];
      if (!f.valid())
        return;
      auto r = f();
      if (!r.valid())
      {
        sol::error err = r;
        SDL_Log("[Scene:%s] error: %s", name.c_str(), err.what());
      }
    }

    void CallEnvHook(const std::string &name, float dt)
    {
      sol::protected_function f = m_Env[name];
      if (!f.valid())
        return;
      auto r = f(dt);
      if (!r.valid())
      {
        sol::error err = r;
        SDL_Log("[Scene:%s] error: %s", name.c_str(), err.what());
      }
    }

    // ── Event table builder ───────────────────────────────────────────────
    // Converts SDL_Event to a plain Lua table.
    // Returns an invalid table if the event type is not exposed to scripts.
    sol::table BuildEventTable(SDL_Event &event)
    {
      sol::state_view lua(m_Env.lua_state());

      switch (event.type)
      {
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
      {
        sol::table t = lua.create_table();
        t["type"] = (event.type == SDL_EVENT_KEY_DOWN) ? "keydown" : "keyup";
        t["key"] = std::string(SDL_GetScancodeName(event.key.scancode));
        t["repeat"] = event.key.repeat;
        return t;
      }
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
      {
        sol::table t = lua.create_table();
        t["type"] = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                        ? "mousedown"
                        : "mouseup";
        t["button"] = (int)event.button.button;
        t["x"] = event.button.x;
        t["y"] = event.button.y;
        return t;
      }
      case SDL_EVENT_MOUSE_MOTION:
      {
        sol::table t = lua.create_table();
        t["type"] = "mousemove";
        t["x"] = event.motion.x;
        t["y"] = event.motion.y;
        t["dx"] = event.motion.xrel;
        t["dy"] = event.motion.yrel;
        return t;
      }
      case SDL_EVENT_MOUSE_WHEEL:
      {
        sol::table t = lua.create_table();
        t["type"] = "mousewheel";
        t["x"] = event.wheel.x;
        t["y"] = event.wheel.y;
        return t;
      }
      default:
        return sol::table{}; // not exposed
      }
    }
  };

} // namespace Cadmium

#endif // CADMIUM_SCRIPT_UPDATE_LAYER_HPP
