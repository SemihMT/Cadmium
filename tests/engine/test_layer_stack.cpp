#include <catch2/catch_test_macros.hpp>
#include <cadmium/core/layer_stack.hpp>
#include <cadmium/core/layer.hpp>
#include <cadmium/core/engine_context.hpp>
#include <memory>
#include <string>
#include <vector>

// -----------------------------------------------------------------------
// Mock context
// -----------------------------------------------------------------------
namespace Cadmium
{
  class Scene;
  class MockContext : public Cadmium::IEngineContext
  {
  public:
    void RequestQuit() override { quitRequested = true; }
    int GetWidth() const override { return 1280; }
    int GetHeight() const override { return 720; }
    Scene* GetActiveScene() override {};
    void PushLayer(std::unique_ptr<Cadmium::Layer> layer) override
    {
      m_Stack.RequestPushLayer(std::move(layer));
    }
    void PushOverlay(std::unique_ptr<Cadmium::Layer> layer) override
    {
      m_Stack.RequestPushOverlay(std::move(layer));
    }
    void PopLayer(const std::string &name) override
    {
      m_Stack.RequestPopLayer(name);
    }
    void PopOverlay(const std::string &name) override
    {
      m_Stack.RequestPopOverlay(name);
    }
    Cadmium::EventBus &GetEventBus() override
    {
    }
    void PushScene(std::unique_ptr<Cadmium::Scene> scene) override {};
    void PopScene() override {};
    void ReplaceScene(std::unique_ptr<Cadmium::Scene> scene) override {};

    bool quitRequested{false};

  private:
    Cadmium::LayerStack m_Stack;
    // Cadmium::EventBus m_Bus;
  };

  // -----------------------------------------------------------------------
  // Instrumented layer
  // -----------------------------------------------------------------------

  struct TrackingLayer : public Cadmium::Layer
  {
    explicit TrackingLayer(const std::string &name, std::vector<std::string> &log)
        : Layer(name), m_Log(log) {}

    void OnAttach() override { m_Log.push_back(GetName() + ":attached"); }
    void OnDetach() override { m_Log.push_back(GetName() + ":detached"); }
    void OnUpdate(float) override { m_Log.push_back(GetName() + ":update"); }
    void OnRender(SDL_Renderer *) override { m_Log.push_back(GetName() + ":render"); }
    void OnEvent(SDL_Event &) override { m_Log.push_back(GetName() + ":event"); }

    std::vector<std::string> &m_Log;
  };

  static std::unique_ptr<TrackingLayer> MakeLayer(
      const std::string &name,
      std::vector<std::string> &log)
  {
    return std::make_unique<TrackingLayer>(name, log);
  }

  // -----------------------------------------------------------------------
  // Tests
  // -----------------------------------------------------------------------

  TEST_CASE("LayerStack - PushLayer calls OnAttach immediately", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("A", log), &ctx);

    REQUIRE(log.size() == 1);
    REQUIRE(log[0] == "A:attached");
  }

  TEST_CASE("LayerStack - PopLayer calls OnDetach", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("A", log), &ctx);
    log.clear();

    stack.PopLayer("A");

    REQUIRE(log.size() == 1);
    REQUIRE(log[0] == "A:detached");
  }

  TEST_CASE("LayerStack - PopLayer on unknown name does nothing", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("A", log), &ctx);
    log.clear();

    stack.PopLayer("nonexistent");

    REQUIRE(log.empty());
  }

  TEST_CASE("LayerStack - layers iterate bottom to top", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("A", log), &ctx);
    stack.PushLayer(MakeLayer("B", log), &ctx);
    stack.PushLayer(MakeLayer("C", log), &ctx);
    log.clear();

    for (auto &layer : stack)
      layer->OnUpdate(0.0f);

    REQUIRE(log[0] == "A:update");
    REQUIRE(log[1] == "B:update");
    REQUIRE(log[2] == "C:update");
  }

  TEST_CASE("LayerStack - layers iterate top to bottom in reverse", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("A", log), &ctx);
    stack.PushLayer(MakeLayer("B", log), &ctx);
    stack.PushLayer(MakeLayer("C", log), &ctx);
    log.clear();

    SDL_Event event{};
    for (auto it = stack.rbegin(); it != stack.rend(); ++it)
      (*it)->OnEvent(event);

    REQUIRE(log[0] == "C:event");
    REQUIRE(log[1] == "B:event");
    REQUIRE(log[2] == "A:event");
  }

  TEST_CASE("LayerStack - overlays sit above layers", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("Game", log), &ctx);
    stack.PushOverlay(MakeLayer("HUD", log), &ctx);
    stack.PushLayer(MakeLayer("Physics", log), &ctx);
    log.clear();

    for (auto &layer : stack)
      layer->OnUpdate(0.0f);

    // Layers below overlay boundary: Game, Physics
    // Overlays above: HUD
    REQUIRE(log[0] == "Game:update");
    REQUIRE(log[1] == "Physics:update");
    REQUIRE(log[2] == "HUD:update");
  }

  TEST_CASE("LayerStack - PopOverlay calls OnDetach and removes", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("Game", log), &ctx);
    stack.PushOverlay(MakeLayer("HUD", log), &ctx);
    log.clear();

    stack.PopOverlay("HUD");

    REQUIRE(log[0] == "HUD:detached");

    log.clear();
    for (auto &layer : stack)
      layer->OnUpdate(0.0f);

    REQUIRE(log.size() == 1);
    REQUIRE(log[0] == "Game:update");
  }

  TEST_CASE("LayerStack - Clear calls OnDetach on all layers", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("A", log), &ctx);
    stack.PushLayer(MakeLayer("B", log), &ctx);
    stack.PushOverlay(MakeLayer("HUD", log), &ctx);
    log.clear();

    stack.Clear();

    REQUIRE(log.size() == 3);
    // All three detached, order not guaranteed so just check membership
    REQUIRE(std::find(log.begin(), log.end(), "A:detached") != log.end());
    REQUIRE(std::find(log.begin(), log.end(), "B:detached") != log.end());
    REQUIRE(std::find(log.begin(), log.end(), "HUD:detached") != log.end());
  }

  TEST_CASE("LayerStack - deferred push via RequestPushLayer", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.RequestPushLayer(MakeLayer("A", log));

    // Not attached yet
    REQUIRE(log.empty());

    stack.FlushPending(&ctx);

    REQUIRE(log.size() == 1);
    REQUIRE(log[0] == "A:attached");
  }

  TEST_CASE("LayerStack - deferred pop via RequestPopLayer", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("A", log), &ctx);
    log.clear();

    stack.RequestPopLayer("A");

    // Not detached yet
    REQUIRE(log.empty());

    stack.FlushPending(&ctx);

    REQUIRE(log.size() == 1);
    REQUIRE(log[0] == "A:detached");
  }

  TEST_CASE("LayerStack - deferred push preserves overlay boundary", "[layer_stack]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    stack.PushLayer(MakeLayer("Game", log), &ctx);
    stack.PushOverlay(MakeLayer("HUD", log), &ctx);

    stack.RequestPushLayer(MakeLayer("Physics", log));
    stack.FlushPending(&ctx);
    log.clear();

    for (auto &layer : stack)
      layer->OnUpdate(0.0f);

    REQUIRE(log[0] == "Game:update");
    REQUIRE(log[1] == "Physics:update");
    REQUIRE(log[2] == "HUD:update");
  }

  TEST_CASE("Layer - context accessible after attach", "[layer]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;
    std::vector<std::string> log;

    bool contextValid = false;
    struct ContextCheckLayer : public Cadmium::Layer
    {
      bool &valid;
      explicit ContextCheckLayer(bool &v) : Layer("Check"), valid(v) {}
      void OnAttach() override
      {
        valid = (GetWidth() == 1280 && GetHeight() == 720);
      }
    };

    stack.PushLayer(std::make_unique<ContextCheckLayer>(contextValid), &ctx);

    REQUIRE(contextValid);
  }

  TEST_CASE("Layer - Quit forwards to context", "[layer]")
  {
    MockContext ctx;
    Cadmium::LayerStack stack;

    struct QuittingLayer : public Cadmium::Layer
    {
      QuittingLayer() : Layer("Quitter") {}
      void OnAttach() override { Quit(); }
    };

    stack.PushLayer(std::make_unique<QuittingLayer>(), &ctx);

    REQUIRE(ctx.quitRequested);
  }
}
