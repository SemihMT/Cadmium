#ifndef CADMIUM_SCRIPTING_SCRIPT_HOST
#define CADMIUM_SCRIPTING_SCRIPT_HOST

#include <cadmium/core/input_manager.hpp>
#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/ecs/world.hpp>
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
            RegisterCoreTypes();
        }

        void Configure(InputManager& input, World& world)
        {
            RegisterInputFunctions(input);
            RegisterEntityFunctions(world);
        }

        void UpdateTime(float dt)
        {
            m_ElapsedTime += dt;
            sol::table time = m_Lua["Time"];
            time["dt"] = dt;
            time["elapsed"] = m_ElapsedTime;
        }

        struct LoadedScript
        {
            std::string path;
            std::string name;

            sol::environment env;

            sol::protected_function onStart;
            sol::protected_function onUpdate;
            sol::protected_function onDestroy;

            bool valid = false;
        };

        LoadedScript LoadScript(const std::string& path)
        {
            LoadedScript loaded;
            loaded.path = path;

            sol::load_result chunk = m_Lua.load_file(path);
            if (!chunk.valid())
            {
                sol::error err = chunk;
                Log::Error("ScriptHost", "Failed to load '{}': {}", path, err.what());
                return loaded;
            }

            // Create a fresh environment for this script instance.
            // Reads fall back to the global Lua state, while writes remain isolated
            // inside this environment.
            loaded.env = sol::environment(m_Lua, sol::create, m_Lua.globals());

            // Bind the environment before executing the chunk.
            sol::protected_function script = chunk;
            sol::set_environment(loaded.env, script);

            // Execute the script once so it can initialize globals and define callbacks.
            sol::protected_function_result result = script();
            if (!result.valid())
            {
                sol::error err = result;
                Log::Error("ScriptHost", "Error executing '{}': {}", path, err.what());
                return loaded;
            }

            loaded.name = loaded.env["Name"].get_or<std::string>("Unnamed");

            loaded.onStart = loaded.env["OnStart"];
            loaded.onUpdate = loaded.env["OnUpdate"];
            loaded.onDestroy = loaded.env["OnDestroy"];

            loaded.valid = true;
            return loaded;
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

        void RegisterCoreTypes()
        {
            sol::table time = m_Lua.create_named_table("Time");
            time["dt"] = 0.0f;
            time["elapsed"] = 0.0f;

            m_Lua.new_usertype<Transform>(
                "Transform",
                "x",
                sol::property(&Transform::GetX, &Transform::SetX),
                "y",
                sol::property(&Transform::GetY, &Transform::SetY),
                "z",
                sol::property(&Transform::GetZ, &Transform::SetZ),

                "scaleX",
                sol::property(&Transform::GetScaleX, &Transform::SetScaleX),
                "scaleY",
                sol::property(&Transform::GetScaleY, &Transform::SetScaleY),
                "scaleZ",
                sol::property(&Transform::GetScaleZ, &Transform::SetScaleZ),

                "rotation",
                sol::property(&Transform::GetRotation, &Transform::SetRotation),
                "rotationX",
                sol::property(&Transform::GetRotationX, &Transform::SetRotationX),
                "rotationY",
                sol::property(&Transform::GetRotationY, &Transform::SetRotationY),
                "rotationZ",
                sol::property(&Transform::GetRotationZ, &Transform::SetRotationZ));

            m_Lua.new_usertype<EntityHandle>(
                "Entity",
                "GetTransform",
                [](EntityHandle& self) -> Transform&
                {
                    if (!self.world->IsValid(self.entity))
                        throw std::runtime_error(
                            "GetTransform called on an invalid or destroyed entity");
                    return self.world->GetComponent<Transform>(self.entity);
                },
                "IsValid",
                [](EntityHandle& self) -> bool { return self.world->IsValid(self.entity); },
                "GetScript",
                [](EntityHandle& self, const std::string& name) -> sol::object
                {
                    if (!self.world->IsValid(self.entity))
                        return sol::lua_nil;
                    if (!self.world->HasComponent<Script>(self.entity))
                        return sol::lua_nil;

                    auto& script = self.world->GetComponent<Script>(self.entity);
                    for (auto& instance : script.instances)
                        if (instance.name == name && instance.env.valid())
                            return instance.env;

                    return sol::lua_nil;
                },
                "Destroy",
                [](EntityHandle& self) -> void { self.world->DestroyEntity(self.entity); });
        }
        void RegisterInputFunctions(InputManager& input)
        {
            m_Lua.set_function("IsKeyDown",
                               [&input](int scancode) -> bool
                               { return input.IsKeyDown(static_cast<SDL_Scancode>(scancode)); });
        }
        void RegisterEntityFunctions(World& world)
        {
            m_Lua.set_function("FindEntityByTag",
                               [this, &world](const std::string& tag) -> sol::object
                               {
                                   for (auto entity : world.QueryEntities<Tag>())
                                   {
                                       auto& t = world.GetComponent<Tag>(entity);
                                       if (t.name == tag)
                                           return sol::make_object(m_Lua,
                                                                   EntityHandle{&world, entity});
                                   }
                                   return sol::lua_nil;
                               });
        }
        sol::state m_Lua;
        double m_ElapsedTime;
    };

} // namespace Cadmium
#endif // CADMIUM_SCRIPTING_SCRIPT_HOST
