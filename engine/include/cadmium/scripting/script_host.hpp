#ifndef CADMIUM_SCRIPTING_SCRIPT_HOST
#define CADMIUM_SCRIPTING_SCRIPT_HOST

#include <cadmium/assets/asset_manager.hpp>
#include <cadmium/core/draw_command_queue.hpp>
#include <cadmium/core/input_manager.hpp>
#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/ecs/world.hpp>
#include <memory>
#include <sol/forward.hpp>
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

        void Configure(InputManager& input,
                       World& world,
                       DrawCommandQueue& drawQueue,
                       AssetManager& assets)
        {
            RegisterInputFunctions(input);
            RegisterEntityFunctions(world);
            RegisterDrawFunctions(drawQueue);
            RegisterAssetFunctions(assets);
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
            sol::environment env;
            std::string path;
            std::string name;

            sol::protected_function onStart;
            sol::protected_function onUpdate;
            sol::protected_function onRender;
            sol::protected_function onDestroy;
            std::unordered_map<std::string, FieldMetadata> fieldMetadata;
            std::vector<std::string> fieldOrder;
            bool valid{false};
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

            sol::environment env{m_Lua, sol::create, m_Lua.globals()};

            auto pending = std::make_shared<PendingState>();
            auto metadata = std::make_shared<std::unordered_map<std::string, FieldMetadata>>();

            env.set_function("Range",
                             [pending, path](double min, double max, sol::optional<double> step)
                             {
                                 if (pending->hasRange)
                                     Log::Warn(
                                         "ScriptHost",
                                         "'{}': Range() called twice before a field assignment, "
                                         "the first call was dropped",
                                         path);

                                 pending->metadata.min = min;
                                 pending->metadata.max = max;
                                 pending->metadata.step = step.value_or(1.0);
                                 pending->hasRange = true;
                             });

            env.set_function("Tooltip",
                             [pending, path](const std::string& text)
                             {
                                 if (pending->hasTooltip)
                                     Log::Warn(
                                         "ScriptHost",
                                         "'{}': Tooltip() called twice before a field assignment,"
                                         "the first call was dropped",
                                         path);

                                 pending->metadata.tooltip = text;
                                 pending->hasTooltip = true;
                             });

            env.set_function("Widget",
                             [pending, path](const std::string& kind)
                             {
                                 if (pending->hasWidget)
                                     Log::Warn(
                                         "ScriptHost",
                                         "'{}': Widget() called twice before a field assignment,"
                                         "the first call was dropped",
                                         path);

                                 pending->metadata.widget = kind;
                                 pending->hasWidget = true;
                             });

            sol::table meta = m_Lua.create_table();
            meta[sol::meta_function::index] = m_Lua.globals();
            auto fieldOrder = std::make_shared<std::vector<std::string>>();
            meta[sol::meta_function::new_index] =
                [pending, metadata, fieldOrder](sol::table t, std::string key, sol::object value)
            {
                t.raw_set(key, value); // always store the assignment

                if (std::find(fieldOrder->begin(), fieldOrder->end(), key) == fieldOrder->end())
                {
                    if (key != "Name" && key != "OnStart" && key != "OnUpdate" &&
                        key != "OnRender" && key != "OnDestroy" && key != "Range" &&
                        key != "Tooltip" && key != "Widget")
                        fieldOrder->push_back(key);
                }

                if (pending->HasAny())
                {
                    (*metadata)[key] = pending->metadata;
                    *pending = PendingState{};
                }
            };
            env[sol::metatable_key] = meta;

            // Bind the environment before executing the chunk.
            sol::protected_function script = chunk;
            sol::set_environment(env, script);

            // Execute the script once so it can initialize globals and define callbacks.
            sol::protected_function_result result = script();
            if (!result.valid())
            {
                sol::error err = result;
                Log::Error("ScriptHost", "Error executing '{}': {}", path, err.what());
                return loaded;
            }
            // script executes succesfully but hasPending is still true -> There is a modifier call
            // that did not apply to a field
            if (pending->HasAny())
                Log::Warn("ScriptHost",
                          "'{}': a modifier was declared but never followed by a "
                          "field assignment, metadata was dropped",
                          path);
            loaded.env = env;
            loaded.name = loaded.env["Name"].get_or<std::string>("Unnamed");
            loaded.onStart = loaded.env["OnStart"];
            loaded.onUpdate = loaded.env["OnUpdate"];
            loaded.onRender = loaded.env["OnRender"];
            loaded.onDestroy = loaded.env["OnDestroy"];
            loaded.fieldMetadata = std::move(*metadata);
            loaded.fieldOrder = std::move(*fieldOrder);
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
        struct PendingState
        {
            FieldMetadata metadata;
            bool hasRange = false;
            bool hasTooltip = false;
            bool hasWidget = false;

            bool HasAny() const { return hasRange || hasTooltip || hasWidget; }
        };
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

            m_Lua.new_usertype<Color>(
                "Color",
                sol::constructors<Color(), Color(float, float, float, float)>(),
                "r",
                &Color::r,
                "g",
                &Color::g,
                "b",
                &Color::b,
                "a",
                &Color::a);

            sol::table colors = m_Lua.create_named_table("Colors");
            colors["White"] = Color::White();
            colors["Black"] = Color::Black();
            colors["Red"] = Color::Red();
            colors["Green"] = Color::Green();
            colors["Blue"] = Color::Blue();
            colors["Yellow"] = Color::Yellow();
            colors["Cyan"] = Color::Cyan();
            colors["Magenta"] = Color::Magenta();
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

        void RegisterDrawFunctions(DrawCommandQueue& queue)
        {
            m_Lua.set_function(
                "DrawLine",
                [&queue](float x1, float y1, float x2, float y2, sol::optional<Color> color)
                { queue.Push(DrawCmd::Line{x1, y1, x2, y2, color.value_or(Color::White())}); });

            m_Lua.set_function(
                "DrawRect",
                [&queue](float x,
                         float y,
                         float w,
                         float h,
                         sol::optional<Color> color,
                         sol::optional<bool> filled)
                {
                    queue.Push(DrawCmd::Rect{
                        x, y, w, h, color.value_or(Color::White()), filled.value_or(false)});
                });

            m_Lua.set_function("DrawCircle",
                               [&queue](float x,
                                        float y,
                                        float radius,
                                        sol::optional<Color> color,
                                        sol::optional<bool> filled,
                                        sol::optional<int> segments)
                               {
                                   queue.Push(DrawCmd::Circle{x,
                                                              y,
                                                              radius,
                                                              segments.value_or(0),
                                                              color.value_or(Color::White()),
                                                              filled.value_or(false)});
                               });

            m_Lua.set_function(
                "DrawPolygon",
                [&queue](sol::table points, sol::optional<Color> color, sol::optional<bool> filled)
                {
                    std::vector<glm::vec2> pts;
                    pts.reserve(points.size());
                    for (auto& kv : points)
                    {
                        sol::table p = kv.second;
                        pts.push_back(glm::vec2{p.get_or("x", 0.0f), p.get_or("y", 0.0f)});
                    }
                    queue.Push(DrawCmd::Polygon{
                        std::move(pts), color.value_or(Color::White()), filled.value_or(false)});
                });

            m_Lua.set_function("DrawSprite",
                               [&queue](TextureHandle handle,
                                        float x,
                                        float y,
                                        sol::optional<float> w,
                                        sol::optional<float> h,
                                        sol::optional<float> rotation,
                                        sol::optional<Color> color,
                                        sol::optional<bool> flipX,
                                        sol::optional<bool> flipY)
                               {
                                   DrawCmd::Sprite cmd{};
                                   cmd.textureHandle = handle;
                                   cmd.x = x;
                                   cmd.y = y;
                                   cmd.w = w.value_or(
                                       0.0f); // 0 = natural texture size, resolved at draw time
                                   cmd.h = h.value_or(0.0f);
                                   cmd.rotation = rotation.value_or(0.0f);
                                   cmd.color = color.value_or(Color::White());
                                   cmd.flipX = flipX.value_or(false);
                                   cmd.flipY = flipY.value_or(false);
                                   queue.Push(cmd);
                               });

            m_Lua.set_function(
                "DrawText",
                [&queue](const std::string& str,
                         float x,
                         float y,
                         sol::optional<float> size,
                         sol::optional<Color> color)
                {
                    queue.Push(DrawCmd::Text{
                        str, x, y, size.value_or(16.0f), color.value_or(Color::White())});
                });

            m_Lua.set_function("SetCamera",
                               [&queue](float x, float y, sol::optional<float> zoom)
                               { queue.Push(DrawCmd::SetCamera{x, y, zoom.value_or(1.0f)}); });

            m_Lua.set_function("ResetCamera", [&queue]() { queue.Push(DrawCmd::ResetCamera{}); });
        }
        void RegisterAssetFunctions(AssetManager& assets)
        {
            sol::table assetsTable = m_Lua.create_named_table("Assets");

            assetsTable.set_function("LoadTexture",
                                     [&assets](const std::string& path) -> TextureHandle
                                     { return assets.LoadTexture(path); });
        }
        sol::state m_Lua;
        double m_ElapsedTime;
    };

} // namespace Cadmium
#endif // CADMIUM_SCRIPTING_SCRIPT_HOST
