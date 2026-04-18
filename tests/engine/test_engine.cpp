#include <catch2/catch_all.hpp>
#include <cadmium/core/engine.hpp>
#include <cadmium/core/application.hpp>

// Minimal concrete Application for testing
class TestApp : public Cadmium::Application
{
public:
  void OnRender() override {}
};

TEST_CASE("Engine - constructs and destructs without throwing", "[engine]")
{
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_VIDEODRIVER", "dummy", 1);
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_AUDIODRIVER", "dummy", 1);
  REQUIRE_NOTHROW(Cadmium::Engine(std::make_unique<TestApp>(), "Test", 800, 600));
}
