#pragma once
#include <SDL3/SDL.h>
#include <variant>
#include <vector>
#include <string>

namespace Cadmium
{
    struct Color
    {
        float r = 1.f, g = 1.f, b = 1.f, a = 1.f;

        static Color White() { return {1, 1, 1, 1}; }
        static Color Black() { return {0, 0, 0, 1}; }
        static Color Red() { return {1, 0, 0, 1}; }
        static Color Green() { return {0, 1, 0, 1}; }
        static Color Blue() { return {0, 0, 1, 1}; }
        static Color Yellow() { return {1, 1, 0, 1}; }
        static Color Cyan() { return {0, 1, 1, 1}; }
        static Color Magenta() { return {1, 0, 1, 1}; }
        static Color Gray(float v) { return {v, v, v, 1}; }
        static Color RGBA(float r, float g, float b, float a) { return {r, g, b, a}; }
        static Color RGB(float r, float g, float b) { return {r, g, b, 1}; }
        static Color Lerp(Color a, Color b, float t)
        {
            return {
                a.r + (b.r - a.r) * t,
                a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t};
        }
    };

    namespace DrawCmd
    {
        struct Line
        {
            float x1, y1, x2, y2;
            Color color;
        };

        struct Rect
        {
            float x, y, w, h;
            Color color;
            bool filled;
        };

        struct Circle
        {
            float x, y, radius;
            int segments; // 0 = auto
            Color color;
            bool filled;
        };

        struct Polygon
        {
            std::vector<SDL_FPoint> points;
            Color color;
            bool filled;
        };

        struct Text
        {
            std::string str;
            float x, y;
            float size;
            Color color;
        };
        struct Sprite
        {
            uint32_t textureHandle;
            float x, y;
            float w, h;     // 0 = use natural texture size
            float rotation; // degrees
            Color color;    // tint
            bool flipX = false;
            bool flipY = false;
        };
        struct SetCamera
        {
            float x, y, zoom;
        };

        struct ResetCamera
        {
        };
    }

    using DrawCommand = std::variant<
        DrawCmd::Line,
        DrawCmd::Rect,
        DrawCmd::Circle,
        DrawCmd::Polygon,
        DrawCmd::Text,
        DrawCmd::Sprite,
        DrawCmd::SetCamera,
        DrawCmd::ResetCamera>;

    // Scripts push commands here during OnRender().
    // ScriptRenderLayer drains and executes them via SDL.
    // Cleared at the end of each drain
    class DrawCommandQueue
    {
    public:
        void Push(DrawCommand cmd) { m_Commands.push_back(std::move(cmd)); }

        const std::vector<DrawCommand> &Commands() const { return m_Commands; }

        void Clear() { m_Commands.clear(); }

        bool Empty() const { return m_Commands.empty(); }

    private:
        std::vector<DrawCommand> m_Commands;
    };

} // namespace Cadmium
