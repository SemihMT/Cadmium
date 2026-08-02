#ifndef SANDBOX_STRESS_RENDER_LAYER
#define SANDBOX_STRESS_RENDER_LAYER

#include <SDL3/SDL.h>
#include <cadmium/core/layer.hpp>


namespace Sandbox
{
    class StressRenderLayer : public Cadmium::Layer
    {
      public:
        StressRenderLayer() : Cadmium::Layer("StressRender") {}
        void OnRender(SDL_Renderer* renderer) override;
    };

} // namespace Sandbox
#endif // SANDBOX_STRESS_RENDER_LAYER
