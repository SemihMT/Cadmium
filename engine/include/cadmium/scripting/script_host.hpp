#ifndef CADMIUM_SCRIPTING_SCRIPT_HOST
#define CADMIUM_SCRIPTING_SCRIPT_HOST

#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/ecs/world.hpp>
#include <cadmium/scripting/script_system.hpp>
#include <sol/sol.hpp>
#include <string>

namespace Cadmium
{
    class ScriptHost
    {
      public:
        ScriptHost()
        {
            m_Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
            InstallPrintOverride();
        }

        // Loads and executes a script file, returning a fresh environment table
        // on success. Never throws — failures are logged and return an
        // invalid (empty) sol::table so the caller can skip creating a
        // Script component for this entity.
        sol::table LoadScript(const std::string& path)
        {
            sol::load_result loaded = m_Lua.load_file(path);
            if (!loaded.valid())
            {
                sol::error err = loaded;
                Log::Error("ScriptHost", "Failed to load '{}': {}", path, err.what());
                return sol::lua_nil;
            }

            sol::protected_function_result executed = loaded();
            if (!executed.valid())
            {
                sol::error err = executed;
                Log::Error("ScriptHost", "Error executing '{}': {}", path, err.what());
                return sol::lua_nil;
            }

            sol::table env = m_Lua.create_table();
            env["OnStart"] = m_Lua["OnStart"];
            env["OnUpdate"] = m_Lua["OnUpdate"];
            env["OnDestroy"] = m_Lua["OnDestroy"];
            return env;
        }

        // Called from Scene::Destroy() before this ScriptHost is destroyed.
        // Sweeps any entities still holding a Script component and gives
        // each one's OnDestroy a chance to run while the interpreter is
        // still alive.
        void Shutdown(World& world)
        {
            for (auto entity : world.QueryEntities<Script>())
                world.DestroyEntity(entity);
        }

        sol::state& GetState() { return m_Lua; }

      private:
        void InstallPrintOverride()
        {
            m_Lua.set_function("print",
                               [this](sol::variadic_args va)
                               {
                                   std::string line;
                                   sol::function tostring = m_Lua["tostring"];
                                   bool first = true;
                                   for (auto v : va)
                                   {
                                       if (!first)
                                           line += '\t';
                                       first = false;
                                       line += tostring(v).get<std::string>();
                                   }
                                   Log::Info("Lua", "{}", line);
                               });
        }

        sol::state m_Lua;
    };

} // namespace Cadmium
#endif // CADMIUM_SCRIPTING_SCRIPT_HOST
