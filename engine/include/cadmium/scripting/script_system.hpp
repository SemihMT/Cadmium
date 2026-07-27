#ifndef CADMIUM_SCRIPTING_SCRIPT_SYSTEM
#define CADMIUM_SCRIPTING_SCRIPT_SYSTEM

#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/ecs/system.hpp>
#include <cadmium/ecs/world.hpp>
#include <cadmium/scripting/script_host.hpp>
#include <sol/error.hpp>
#include <sol/forward.hpp>
#include <sol/protected_function_result.hpp>

namespace Cadmium
{

    template <typename... Args>
    void CallProtected(const std::string& hookName, sol::function fn, Args&&... args)
    {
        if (!fn.valid())
            return;

        sol::protected_function_result result = fn(std::forward<Args>(args)...);
        if (!result.valid())
        {
            sol::error err = result;
            Log::Warn("ScriptSystem", "{} error: {}", hookName, err.what());
        }
    }

    // ECS System that updates all components with a script
    class ScriptSystem : public System
    {
      public:
        void OnUpdate(World& world, float dt) override
        {
            world.GetScriptHost().UpdateTime(dt);
            for (auto entity : world.QueryEntities<Script>())
            {
                auto& script = world.GetComponent<Script>(entity);
                for (auto& instance : script.instances)
                {
                    if (!instance.started)
                    {
                        instance.started = true;
                        CallProtected("OnStart", instance.onStart);
                    }
                    CallProtected("OnUpdate", instance.onUpdate, dt);
                }
            }
        }

        void OnEntityDestroyed(World& world, Entity entity) override
        {
            if (!world.HasComponent<Script>(entity))
                return;

            auto& script = world.GetComponent<Script>(entity);
            for (auto& instance : script.instances)
                CallProtected("OnDestroy", instance.onDestroy);
        }
    };

} // namespace Cadmium

#endif // CADMIUM_SCRIPTING_SCRIPT_SYSTEM
