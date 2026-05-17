// engine/include/cadmium/scripting/script_update_layer.hpp
#ifndef CADMIUM_SCRIPT_UPDATE_LAYER_HPP
#define CADMIUM_SCRIPT_UPDATE_LAYER_HPP

#include <cadmium/core/layer.hpp>
#include <cadmium/scripting/script_host.hpp>
#include <SDL3/SDL.h>

namespace Cadmium
{

class ScriptUpdateLayer : public Layer
{
public:
    explicit ScriptUpdateLayer(ScriptHost& host)
        : Layer("ScriptUpdateLayer"), m_Host(host)
    {}

    void OnUpdate(float dt) override
    {
        if (m_Host.IsPaused()) return;
        CallEnvHook("OnUpdate", dt);

        for (const auto& entry : m_Host.GetRegistry().All())
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

    void OnFixedUpdate(float dt) override
    {
        if (m_Host.IsPaused()) return;
        CallEnvHook("OnFixedUpdate", dt);

        for (const auto& entry : m_Host.GetRegistry().All())
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

    void OnRender(SDL_Renderer*) override
    {
        if (m_Host.IsPaused()) return;
        CallEnvHook("OnRender");

        for (const auto& entry : m_Host.GetRegistry().All())
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
            auto r = hook(entry.handle);
            if (!r.valid())
            {
                sol::error err = r;
                SDL_Log("[Entity:OnRender] error: %s", err.what());
            }
        }

        m_Host.GetRegistry().FlushDestroyed(GetWorld());
    }

    void OnEvent(SDL_Event& event) override
    {
        if (m_Host.IsPaused()) return;

        sol::table evtTable = BuildEventTable(event);
        if (!evtTable.valid())
            return;

        CallEnvHook("OnEvent");

        for (const auto& entry : m_Host.GetRegistry().All())
        {
            sol::object active = entry.table["active"];
            if (active.is<bool>() && !active.as<bool>())
                continue;

            sol::object hookObj = entry.table["OnEvent"];
            if (!hookObj.is<sol::protected_function>())
                continue;

            sol::protected_function hook = hookObj;
            hook(entry.handle, evtTable);
        }
    }

private:
    ScriptHost& m_Host;

    void CallEnvHook(const std::string& name)
    {
        sol::protected_function f = m_Host.GetEnv()[name];
        if (!f.valid()) return;
        auto r = f();
        if (!r.valid())
        {
            sol::error err = r;
            SDL_Log("[Scene:%s] error: %s", name.c_str(), err.what());
        }
    }

    void CallEnvHook(const std::string& name, float dt)
    {
        sol::protected_function f = m_Host.GetEnv()[name];
        if (!f.valid()) return;
        auto r = f(dt);
        if (!r.valid())
        {
            sol::error err = r;
            SDL_Log("[Scene:%s] error: %s", name.c_str(), err.what());
        }
    }

    sol::table BuildEventTable(SDL_Event& event)
    {
        sol::state_view lua(m_Host.GetEnv().lua_state());

        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            sol::table t = lua.create_table();
            t["type"]   = (event.type == SDL_EVENT_KEY_DOWN) ? "keydown" : "keyup";
            t["key"]    = std::string(SDL_GetScancodeName(event.key.scancode));
            t["repeat"] = event.key.repeat;
            return t;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            sol::table t = lua.create_table();
            t["type"]   = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? "mousedown" : "mouseup";
            t["button"] = (int)event.button.button;
            t["x"]      = event.button.x;
            t["y"]      = event.button.y;
            return t;
        }
        case SDL_EVENT_MOUSE_MOTION:
        {
            sol::table t = lua.create_table();
            t["type"] = "mousemove";
            t["x"]    = event.motion.x;
            t["y"]    = event.motion.y;
            t["dx"]   = event.motion.xrel;
            t["dy"]   = event.motion.yrel;
            return t;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            sol::table t = lua.create_table();
            t["type"] = "mousewheel";
            t["x"]    = event.wheel.x;
            t["y"]    = event.wheel.y;
            return t;
        }
        default:
            return sol::table{};
        }
    }
};

} // namespace Cadmium

#endif // CADMIUM_SCRIPT_UPDATE_LAYER_HPP
