#ifndef CADMIUM_SCRIPT_UPDATE_LAYER_HPP
#define CADMIUM_SCRIPT_UPDATE_LAYER_HPP
#include <cadmium/core/layer.hpp>
#include <sol/sol.hpp>

namespace Cadmium
{
  class ScriptUpdateLayer : public Layer
  {
  public:
    explicit ScriptUpdateLayer(sol::environment &env)
        : Layer("ScriptUpdateLayer"), m_Env(env) {}

    void OnUpdate(float dt) override
    {
      sol::protected_function f = m_Env["OnUpdate"];
      if (!f.valid())
        return;
      auto r = f(dt);
      if (!r.valid())
      {
        sol::error err = r;
        SDL_Log("[Lua] OnUpdate error: %s", err.what());
      }
    }

    void OnRender(SDL_Renderer *) override
    {
      sol::protected_function f = m_Env["OnRender"];
      if (!f.valid())
        return;
      auto r = f();
      if (!r.valid())
      {
        sol::error err = r;
        SDL_Log("[Lua] OnRender error: %s", err.what());
      }
    }

    void OnFixedUpdate(float dt) override
    {
      sol::protected_function f = m_Env["OnFixedUpdate"];
      if (!f.valid())
        return;
      auto r = f(dt);
      if (!r.valid())
      {
        sol::error err = r;
        SDL_Log("[Lua] OnFixedUpdate error: %s", err.what());
      }
    }

  private:
    sol::environment &m_Env;
  };
} // namespace Cadmium

#endif // CADMIUM_SCRIPT_UPDATE_LAYER_HPP
