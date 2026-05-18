#ifndef CADMIUM_EDITOR_RENDER_VIEWPORT_HPP
#define CADMIUM_EDITOR_RENDER_VIEWPORT_HPP

#include <SDL3/SDL.h>

#include <memory>

namespace Cadmium::Editor
{

  struct SdlTextureDeleter {
    void operator()(SDL_Texture* tex) const noexcept {
       SDL_DestroyTexture(tex);
    }
};

// Owns an SDL_Texture with SDL_TEXTUREACCESS_TARGET.
// The engine renders into it; the editor panel displays it via ImGui::Image.
class RenderViewport
{
public:
    RenderViewport() = default;
    ~RenderViewport() = default;

    RenderViewport(RenderViewport&&) noexcept = default;
    RenderViewport& operator=(RenderViewport&&) noexcept = default;
    RenderViewport(const RenderViewport&) = delete;
    RenderViewport& operator=(const RenderViewport&) = delete;

    // Call after SDL_Renderer is available.
    // width/height = initial size in pixels.
    bool Init(SDL_Renderer *renderer, int width, int height)
    {
      SDL_assert(m_Renderer == nullptr);
      m_Renderer = renderer;
      return Resize(width, height);
    }

    // Recreate the texture at a new size.
    // Call when the viewport panel is resized.
    bool Resize(int width, int height)
    {
      if (width <= 0 || height <= 0)
        return false;
      if (!m_Renderer)
      {
        SDL_Log("[RenderViewport] Resize called without a renderer.");
        return false;
      }

      if (m_Texture && width == m_Width && height == m_Height)
        return true;

      m_Texture.reset();

      std::unique_ptr<SDL_Texture, SdlTextureDeleter> newTex(SDL_CreateTexture(
          m_Renderer,
          SDL_PIXELFORMAT_RGBA8888,
          SDL_TEXTUREACCESS_TARGET,
          width, height));

      if (!newTex)
      {
        SDL_Log("[RenderViewport] Failed to create render target: %s",
                SDL_GetError());
        m_Width = 0;
        m_Height = 0;
        return false;
      }

      m_Texture = std::move(newTex);
      m_Width = width;
      m_Height = height;
      return true;
    }

    // Redirect SDL rendering to this texture.
    bool Bind() const
    {
      SDL_assert(m_Renderer);
      if (!m_Texture)
        return false;
      if (!SDL_SetRenderTarget(m_Renderer, m_Texture.get()))
      {
        SDL_Log("[RenderViewport] Bind failed: %s", SDL_GetError());
        return false;
      }
      return true;
    }

    // Restore rendering to the default backbuffer.
    bool Unbind() const
    {
      SDL_assert(m_Renderer);
      if (!SDL_SetRenderTarget(m_Renderer, nullptr))
      {
        SDL_Log("[RenderViewport] Unbind failed: %s", SDL_GetError());
        return false;
      }
      return true;
    }

    SDL_Texture* GetTexture() const { return m_Texture.get(); }
    void* GetImTextureID()   const { return static_cast<void*>(m_Texture.get()); }
    int GetWidth()  const { return m_Width; }
    int GetHeight() const { return m_Height; }
    bool IsReady()  const { return m_Texture != nullptr; }

private:
    SDL_Renderer* m_Renderer{nullptr};
    std::unique_ptr<SDL_Texture, SdlTextureDeleter> m_Texture;
    int           m_Width{0};
    int           m_Height{0};
};

} // namespace Cadmium::Editor
#endif // CADMIUM_EDITOR_RENDER_VIEWPORT_HPP
