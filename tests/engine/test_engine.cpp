#include <catch2/catch_all.hpp>
#include <cadmium/core/engine.hpp>
#include <cadmium/core/layer.hpp>

// Minimal concrete Application for testing
class TestApp : public Cadmium::Layer
{
public:
  TestApp() : Layer("TestApp") {}
  void OnRender(SDL_Renderer *renderer) override {}
};

TEST_CASE("Engine - constructs and destructs without throwing", "[engine]")
{
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_VIDEODRIVER", "dummy", 1);
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_AUDIODRIVER", "dummy", 1);
  REQUIRE_NOTHROW([&]()
                  {
    Cadmium::Engine cdEngine("Test", 800, 600);
    cdEngine.PushLayer(std::make_unique<TestApp>()); }());
}
