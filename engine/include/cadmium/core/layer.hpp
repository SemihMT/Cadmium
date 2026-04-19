#ifndef CADMIUM_LAYER_HPP
#define CADMIUM_LAYER_HPP

#include <cadmium/core/engine_context.hpp>
#include <SDL3/SDL.h>
#include <string>

namespace Cadmium
{
  class Layer
  {
  public:
    explicit Layer(std::string name) : m_Name{std::move(name)} {}
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnFixedUpdate(float dt) {}
    virtual void OnRender(SDL_Renderer* renderer) {}
    virtual void OnEvent(SDL_Event& event) {}
    virtual void OnImGuiRender() {}

    const std::string& GetName() const { return m_Name; }

  protected:
    void Quit() { m_Context->RequestQuit(); }
    int GetWidth() const { return m_Context->GetWidth(); }
    int GetHeight() const { return m_Context->GetHeight(); }

  private:
    friend class LayerStack;
    void SetContext(IEngineContext* context) { m_Context = context; }

    std::string m_Name;
    IEngineContext* m_Context{nullptr};
  };
} // namespace Cadmium

#endif // CADMIUM_LAYER_HPP
