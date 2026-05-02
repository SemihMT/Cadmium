#ifndef CADMIUM_LUA_BINDINGS_HPP
#define CADMIUM_LUA_BINDINGS_HPP
#include <cadmium/core/input_manager.hpp>
#include <cadmium/core/draw_command_queue.hpp>
#include <sol/sol.hpp>
#include <string>

// Forward declare so we don't pull in the full engine header here
namespace Cadmium
{
  class IEngineContext;
}

namespace Cadmium::Lua
{

  // ── Math extensions ───────────────────────────────────────────────────────
  // Injected into Lua's existing math table.
  inline void BindMathExtensions(sol::state &lua)
  {
    sol::table math = lua["math"];

    math.set_function("clamp", [](float v, float lo, float hi)
                      { return v < lo ? lo : (v > hi ? hi : v); });

    math.set_function("lerp", [](float a, float b, float t)
                      { return a + (b - a) * t; });

    math.set_function("sign", [](float v) -> float
                      { return v > 0.f ? 1.f : (v < 0.f ? -1.f : 0.f); });

    math.set_function("round", [](float v) -> float
                      { return std::round(v); });
  }

  // ── Color ─────────────────────────────────────────────────────────────────
  // Exposed as a Lua table of constants + constructor functions.
  // Color values are plain Lua tables {r, g, b, a} — no userdata needed.
  // Draw functions accept anything with r/g/b/a fields.
  inline void BindColor(sol::state &lua)
  {
    sol::table color = lua.create_named_table("Color");
    sol::state *L = &lua;

    auto makeColor = [&lua](float r, float g, float b, float a = 1.f)
    {
      sol::table c = lua.create_table();
      c["r"] = r;
      c["g"] = g;
      c["b"] = b;
      c["a"] = a;
      return c;
    };

    color["Red"] = makeColor(1, 0, 0);
    color["Green"] = makeColor(0, 1, 0);
    color["Blue"] = makeColor(0, 0, 1);
    color["White"] = makeColor(1, 1, 1);
    color["Black"] = makeColor(0, 0, 0);
    color["Yellow"] = makeColor(1, 1, 0);
    color["Cyan"] = makeColor(0, 1, 1);
    color["Magenta"] = makeColor(1, 0, 1);

    color.set_function("Gray", [L](float v)
                       {
        sol::table c = L->create_table();
        c["r"] = v; c["g"] = v; c["b"] = v; c["a"] = 1.f;
        return c; });

    color.set_function("RGB", [L](float r, float g, float b)
                       {
        sol::table c = L->create_table();
        c["r"] = r; c["g"] = g; c["b"] = b; c["a"] = 1.f;
        return c; });

    color.set_function("RGBA", [L](float r, float g, float b, float a)
                       {
        sol::table c = L->create_table();
        c["r"] = r; c["g"] = g; c["b"] = b; c["a"] = a;
        return c; });

    color.set_function("Lerp", [L](sol::table a, sol::table b, float t)
                       {
        sol::table c = L->create_table();
        auto lerp = [](float x, float y, float t) { return x + (y - x) * t; };
        c["r"] = lerp(a["r"], b["r"], t);
        c["g"] = lerp(a["g"], b["g"], t);
        c["b"] = lerp(a["b"], b["b"], t);
        c["a"] = lerp(a["a"], b["a"], t);
        return c; });

    color.set_function("Hex", [L](const std::string &hex)
                       {
        sol::table c = L->create_table();
        std::string h = hex;
        if (!h.empty() && h[0] == '#') h = h.substr(1);
        auto hexByte = [&](int pos) -> float {
            return std::stoi(h.substr(pos, 2), nullptr, 16) / 255.f;
        };
        c["r"] = h.size() >= 2 ? hexByte(0) : 1.f;
        c["g"] = h.size() >= 4 ? hexByte(2) : 1.f;
        c["b"] = h.size() >= 6 ? hexByte(4) : 1.f;
        c["a"] = h.size() >= 8 ? hexByte(6) : 1.f;
        return c; });
  }

  // ── Input ─────────────────────────────────────────────────────────────────
  inline void BindInput(sol::state &lua, InputManager &input)
  {
    sol::table tbl = lua.create_named_table("Input");
    sol::state *L = &lua;
    InputManager *inp = &input; // same pattern — pointer instead of reference capture

    tbl.set_function("IsKeyDown", [inp](const std::string &name)
                     { return inp->IsKeyDown(InputManager::ScancodeFromName(name)); });

    tbl.set_function("IsKeyJustPressed", [inp](const std::string &name)
                     { return inp->IsKeyJustPressed(InputManager::ScancodeFromName(name)); });

    tbl.set_function("IsKeyJustReleased", [inp](const std::string &name)
                     { return inp->IsKeyJustReleased(InputManager::ScancodeFromName(name)); });

    tbl.set_function("IsMouseDown", [inp](int btn)
                     { return inp->IsMouseDown(btn); });

    tbl.set_function("IsMouseJustPressed", [inp](int btn)
                     { return inp->IsMouseJustPressed(btn); });

    tbl.set_function("IsMouseJustReleased", [inp](int btn)
                     { return inp->IsMouseJustReleased(btn); });

    tbl.set_function("MousePosition", [inp, L]()
                     {
        sol::table t = L->create_table();
        t["x"] = inp->MouseX();
        t["y"] = inp->MouseY();
        return t; });

    tbl.set_function("MouseDelta", [inp, L]()
                     {
        sol::table t = L->create_table();
        t["x"] = inp->MouseDeltaX();
        t["y"] = inp->MouseDeltaY();
        return t; });

    tbl.set_function("MouseScroll", [inp, L]()
                     {
        sol::table t = L->create_table();
        t["x"] = inp->ScrollX();
        t["y"] = inp->ScrollY();
        return t; });
  }

  // ── Draw ──────────────────────────────────────────────────────────────────
  // All functions push to the DrawCommandQueue.
  // Executed later by ScriptRenderLayer — scripts never touch SDL directly.
  inline void BindDraw(sol::state &lua, DrawCommandQueue &queue)
  {
    sol::table tbl = lua.create_named_table("Draw");
    DrawCommandQueue *q = &queue; // pointer capture

    auto toColor = [](sol::table t) -> Color
    {
      return Color{
          t.get_or("r", 1.f),
          t.get_or("g", 1.f),
          t.get_or("b", 1.f),
          t.get_or("a", 1.f)};
    };

    tbl.set_function("Line",
                     [q, toColor](float x1, float y1, float x2, float y2, sol::table color)
                     {
                       q->Push(DrawCmd::Line{x1, y1, x2, y2, toColor(color)});
                     });

    tbl.set_function("Rect",
                     [q, toColor](float x, float y, float w, float h, sol::table color)
                     {
                       q->Push(DrawCmd::Rect{x, y, w, h, toColor(color), false});
                     });

    tbl.set_function("FilledRect",
                     [q, toColor](float x, float y, float w, float h, sol::table color)
                     {
                       q->Push(DrawCmd::Rect{x, y, w, h, toColor(color), true});
                     });

    tbl.set_function("Circle",
                     [q, toColor](float x, float y, float r, sol::table color)
                     {
                       q->Push(DrawCmd::Circle{x, y, r, 0, toColor(color), false});
                     });

    tbl.set_function("FilledCircle",
                     [q, toColor](float x, float y, float r, sol::table color)
                     {
                       q->Push(DrawCmd::Circle{x, y, r, 0, toColor(color), true});
                     });

    tbl.set_function("Polygon",
                     [q, toColor](sol::table points, sol::table color)
                     {
                       std::vector<SDL_FPoint> pts;
                       for (auto &kv : points)
                       {
                         sol::table p = kv.second.as<sol::table>();
                         pts.push_back({p["x"], p["y"]});
                       }
                       q->Push(DrawCmd::Polygon{std::move(pts), toColor(color), false});
                     });

    tbl.set_function("Text",
                     [q, toColor](const std::string &str, float x, float y,
                                  float size, sol::table color)
                     {
                       q->Push(DrawCmd::Text{str, x, y, size, toColor(color)});
                     });

    tbl.set_function("SetCamera",
                     [q](float x, float y, float zoom)
                     {
                       q->Push(DrawCmd::SetCamera{x, y, zoom});
                     });

    tbl.set_function("ResetCamera", [q]()
                     { q->Push(DrawCmd::ResetCamera{}); });
  }
  // ── Scene ─────────────────────────────────────────────────────────────────
  // Width and Height are updated each frame from the engine context.
  // Time and DeltaTime are updated each frame from the engine loop.
  // Scripts read these as plain values — no function call needed.
  struct SceneBindingState
  {
    float Width = 1280.f;
    float Height = 720.f;
    float Time = 0.f;
    float DeltaTime = 0.f;
  };

  inline void BindScene(sol::state &lua, SceneBindingState &state)
  {
    sol::table tbl = lua.create_named_table("Scene");

    // These are updated from C++ each frame via the state struct.
    // Lua reads them as plain table fields.
    tbl["Width"] = state.Width;
    tbl["Height"] = state.Height;
    tbl["Time"] = state.Time;
    tbl["DeltaTime"] = state.DeltaTime;

    // Called by the engine each frame to keep values current.
    // Not exposed to Lua.
  }

  // Call this every frame to keep Scene.Width/Height/Time/DeltaTime current.
  inline void UpdateSceneBindings(sol::state &lua, const SceneBindingState &state)
  {
    sol::table tbl = lua["Scene"];
    tbl["Width"] = state.Width;
    tbl["Height"] = state.Height;
    tbl["Time"] = state.Time;
    tbl["DeltaTime"] = state.DeltaTime;
  }

  // ── vec2 / vec3 ───────────────────────────────────────────────────────────
  inline void BindMathTypes(sol::state &lua)
  {
    // vec2
    struct vec2
    {
      float x, y;
    };

    lua.new_usertype<vec2>("vec2", sol::call_constructor, sol::constructors<vec2(), vec2(float, float)>(),

                           "x", &vec2::x, "y", &vec2::y,

                           sol::meta_function::addition, [](const vec2 &a, const vec2 &b)
                           { return vec2{a.x + b.x, a.y + b.y}; }, sol::meta_function::subtraction, [](const vec2 &a, const vec2 &b)
                           { return vec2{a.x - b.x, a.y - b.y}; }, sol::meta_function::multiplication, sol::overload([](const vec2 &v, float s)
                                                                                                                     { return vec2{v.x * s, v.y * s}; }, [](float s, const vec2 &v)
                                                                                                                     { return vec2{v.x * s, v.y * s}; }),
                           sol::meta_function::division, [](const vec2 &v, float s)
                           { return vec2{v.x / s, v.y / s}; }, sol::meta_function::unary_minus, [](const vec2 &v)
                           { return vec2{-v.x, -v.y}; }, sol::meta_function::to_string, [](const vec2 &v)
                           { return "vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")"; }, "length", [](const vec2 &v)
                           { return std::sqrt(v.x * v.x + v.y * v.y); }, "lengthSq", [](const vec2 &v)
                           { return v.x * v.x + v.y * v.y; }, "normalize", [](const vec2 &v)
                           {
            float len = std::sqrt(v.x*v.x + v.y*v.y);
            if (len < 1e-8f) return vec2{0,0};
            return vec2{v.x/len, v.y/len}; }, "dot", [](const vec2 &a, const vec2 &b)
                           { return a.x * b.x + a.y * b.y; }, "lerp", [](const vec2 &a, const vec2 &b, float t)
                           { return vec2{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }, "distance", [](const vec2 &a, const vec2 &b)
                           {
            float dx=a.x-b.x, dy=a.y-b.y;
            return std::sqrt(dx*dx+dy*dy); });

    // vec3
    struct vec3
    {
      float x, y, z;
    };

    lua.new_usertype<vec3>("vec3", sol::call_constructor, sol::constructors<vec3(), vec3(float, float, float)>(),

                           "x", &vec3::x, "y", &vec3::y, "z", &vec3::z,

                           sol::meta_function::addition, [](const vec3 &a, const vec3 &b)
                           { return vec3{a.x + b.x, a.y + b.y, a.z + b.z}; }, sol::meta_function::subtraction, [](const vec3 &a, const vec3 &b)
                           { return vec3{a.x - b.x, a.y - b.y, a.z - b.z}; }, sol::meta_function::multiplication, sol::overload([](const vec3 &v, float s)
                                                                                                                                { return vec3{v.x * s, v.y * s, v.z * s}; }, [](float s, const vec3 &v)
                                                                                                                                { return vec3{v.x * s, v.y * s, v.z * s}; }),
                           sol::meta_function::division, [](const vec3 &v, float s)
                           { return vec3{v.x / s, v.y / s, v.z / s}; }, sol::meta_function::unary_minus, [](const vec3 &v)
                           { return vec3{-v.x, -v.y, -v.z}; }, sol::meta_function::to_string, [](const vec3 &v)
                           { return "vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")"; }, "length", [](const vec3 &v)
                           { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }, "normalize", [](const vec3 &v)
                           {
            float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
            if (len < 1e-8f) return vec3{0,0,0};
            return vec3{v.x/len, v.y/len, v.z/len}; }, "dot", [](const vec3 &a, const vec3 &b)
                           { return a.x * b.x + a.y * b.y + a.z * b.z; }, "cross", [](const vec3 &a, const vec3 &b)
                           { return vec3{
                                 a.y * b.z - a.z * b.y,
                                 a.z * b.x - a.x * b.z,
                                 a.x * b.y - a.y * b.x}; }, "lerp", [](const vec3 &a, const vec3 &b, float t)
                           { return vec3{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t}; }, "distance", [](const vec3 &a, const vec3 &b)
                           {
            float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
            return std::sqrt(dx*dx+dy*dy+dz*dz); });
  }

  // ── BindAll ───────────────────────────────────────────────────────────────
  // Call once after opening Lua libraries.
  inline void BindPhase1(sol::state &lua,
                         InputManager &input,
                         DrawCommandQueue &queue,
                         SceneBindingState &scene)
  {
    BindMathTypes(lua);
    BindMathExtensions(lua);
    BindColor(lua);
    BindInput(lua, input);
    BindDraw(lua, queue);
    BindScene(lua, scene);
  }

} // namespace Cadmium::Lua
#endif // CADMIUM_LUA_BINDINGS_HPP
