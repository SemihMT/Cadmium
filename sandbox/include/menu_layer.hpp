#ifndef SANDBOX_MENU_LAYER
#define SANDBOX_MENU_LAYER

#include <SDL3/SDL.h>
#include <cadmium/core/layer.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Sandbox
{
    class AsteroidsScene;
    class StressScene;

    class MenuLayer : public Cadmium::Layer
    {
      public:
        MenuLayer() : Cadmium::Layer("Menu") {}
        void OnImGuiRender() override;
        void OnEvent(SDL_Event& event) override;
    };

} // namespace Sandbox
#endif // SANDBOX_MENU_LAYER
