#ifndef CADMIUM_SCRIPTING_SCRIPT_SYSTEM
#define CADMIUM_SCRIPTING_SCRIPT_SYSTEM

#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/ecs/system.hpp>
#include <cadmium/ecs/world.hpp>
#include <sol/error.hpp>
#include <sol/forward.hpp>
#include <sol/protected_function_result.hpp>

namespace Cadmium
{
    // ECS System that updates all components with a script
    class ScriptSystem : public System
    {
      public:
        void OnUpdate(World& world, float dt) override
        {
            for (auto entity : world.QueryEntities<Script>())
            {
                auto& script = world.GetComponent<Script>(entity);

                if (!script.started)
                {
                    script.started = true;
                    if (script.onStart.valid())
                    {
                        sol::protected_function_result result = script.onStart(script.self);
                        if (!result.valid())
                        {
                            sol::error err = result;
                            Log::Warn("ScriptSystem", "OnStart error: {}", err.what());
                        }
                    }
                }

                if (!script.onUpdate.valid())
                    continue;

                sol::protected_function_result result = script.onUpdate(script.self, dt);
                if (!result.valid())
                {
                    sol::error err = result;
                    Log::Warn("ScriptSystem", "OnUpdate error: {}", err.what());
                }
            }
        }

        void OnEntityDestroyed(World& world, Entity entity) override
        {
            if (!world.HasComponent<Script>(entity))
                return;

            auto& script = world.GetComponent<Script>(entity);
            if (!script.onDestroy.valid())
                return;

            sol::protected_function_result result = script.onDestroy(script.self);
            if (!result.valid())
            {
                sol::error err = result;
                Log::Warn("ScriptSystem", "OnDestroy error: {}", err.what());
            }
        }
    };

} // namespace Cadmium

#endif // CADMIUM_SCRIPTING_SCRIPT_SYSTEM
