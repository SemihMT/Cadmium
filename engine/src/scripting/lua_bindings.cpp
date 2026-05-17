#include <cadmium/ecs/entity.hpp>
#include <cadmium/ecs/registry.hpp>
#include <cadmium/ecs/world.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/scripting/lua_bindings_entity.hpp>
#include <cadmium/scripting/lua_bindings_components.hpp>
#include <SDL3/SDL.h>

namespace Cadmium::Lua
{

void RegisterEntityTypes(sol::state& lua)
{
    lua.new_usertype<EntityHandle>("EntityHandle", sol::no_constructor,

    sol::meta_function::index,
    [](EntityHandle& self, const std::string& key, sol::this_state s) -> sol::object
    {
        sol::state_view lua(s);
        if (k_TransformFields.count(key))
        {
            Transform* t = self.GetTransform();
            if (!t) return sol::make_object(lua, 0.f);
            return sol::make_object(lua, self.GetField(key));
        }
        return self.table[key];
    },

    sol::meta_function::new_index,
    [](EntityHandle& self, const std::string& key, sol::object value)
    {
        if (k_TransformFields.count(key))
        {
            float v = 0.f;
            if (value.is<float>())       v = value.as<float>();
            else if (value.is<int>())    v = (float)value.as<int>();
            else
            {
                SDL_Log("[Entity] '%s' requires a number", key.c_str());
                return;
            }
            self.SetField(key, v);
            return;
        }
        if (k_EngineFields.count(key)) return;
        self.table[key] = value;
    },

    sol::meta_function::to_string,
    [](EntityHandle& self) -> std::string
    {
        sol::object name = self.table["name"];
        if (name.is<std::string>())
            return "Entity(" + name.as<std::string>() + ")";
        return "Entity";
    },

    sol::meta_function::equal_to,
    [](const EntityHandle& a, const EntityHandle& b)
    {
        return a.cppEntity == b.cppEntity;
    });
}

void RegisterComponentTypes(sol::state& lua)
{
    lua.new_usertype<LuaComponentProxy>("LuaComponentProxy", sol::no_constructor,

    sol::meta_function::index,
    [](LuaComponentProxy& self, const std::string& key, sol::this_state s) -> sol::object
    {
        sol::state_view L(s);
        if (!self.data) return sol::nil;
        auto it = self.data->values.find(key);
        if (it == self.data->values.end()) return sol::nil;
        return std::visit([&](const auto& v) {
            return sol::make_object(L, v);
        }, it->second);
    },

    sol::meta_function::new_index,
    [](LuaComponentProxy& self, const std::string& key, sol::object value)
    {
        if (!self.data || !self.data->schema) return;
        auto fieldIt = self.data->schema->fields.find(key);
        if (fieldIt == self.data->schema->fields.end())
        {
            SDL_Log("[Component] Unknown field '%s' on '%s'",
                    key.c_str(), self.typeName.c_str());
            return;
        }
        switch (fieldIt->second.type)
        {
        case LuaFieldType::Float:
            if (value.is<float>())      self.data->values[key] = value.as<float>();
            else if (value.is<int>())   self.data->values[key] = (float)value.as<int>();
            break;
        case LuaFieldType::Bool:
            if (value.is<bool>())       self.data->values[key] = value.as<bool>();
            break;
        case LuaFieldType::String:
            if (value.is<std::string>()) self.data->values[key] = value.as<std::string>();
            break;
        }
    },

    sol::meta_function::to_string,
    [](const LuaComponentProxy& self)
    {
        return "LuaComponent(" + self.typeName + ")";
    });
}

} // namespace Cadmium::Lua
