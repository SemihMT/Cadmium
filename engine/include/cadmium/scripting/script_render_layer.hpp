#ifndef CADMIUM_SCRIPT_RENDER_LAYER
#define CADMIUM_SCRIPT_RENDER_LAYER
#include "cadmium/render/renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cadmium/core/draw_command_queue.hpp>
#include <cadmium/core/layer.hpp>
#include <cadmium/ecs/world.hpp>
#include <cadmium/scripting/script_system.hpp>

namespace Cadmium
{
    // Push after RegisterSystem<ScriptSystem>() in a scene's OnEnter():
    //
    //   RegisterSystem<Cadmium::ScriptSystem>(0);
    //   PushLayer(std::make_unique<Cadmium::ScriptRenderLayer>());
    //
    // Each frame: runs every script instance's OnRender() (letting them push
    // commands), then drains and executes the queue via SDL, then clears it.
    class ScriptRenderLayer : public Layer
    {
      public:
        ScriptRenderLayer() : Layer("ScriptRender") {}

        void OnRender(SDL_Renderer*) override
        {
            World& world = GetWorld();
            if (world.HasSystem<ScriptSystem>())
                world.GetSystem<ScriptSystem>().OnRender(world);

            IRenderer& renderer = GetRenderer();
            DrawCommandQueue& queue = GetDrawQueue();
            for (const auto& cmd : queue.Commands())
                std::visit([&](auto&& c) { DispatchDrawCommand(renderer, c); }, cmd);
            queue.Clear();
        }
    };

} // namespace Cadmium
#endif // CADMIUM_SCRIPT_RENDER_LAYER
