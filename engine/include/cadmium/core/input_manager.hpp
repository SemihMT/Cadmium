#ifndef CADMIUM_INPUT_MANAGER_HPP
#define CADMIUM_INPUT_MANAGER_HPP
#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <string>

namespace Cadmium
{
class InputManager
{
public:
    // Copies current state to previous state.
    void BeginFrame();

    // Snapshots the current SDL keyboard state into m_Current.
    void SnapshotPost();

    // Feed scroll events in from the engine's event loop.
    void OnMouseWheel(float x, float y);

    // ── Keyboard ─────────────────────────────────────────────────────────

    // True every frame the key is held.
    bool IsKeyDown(SDL_Scancode sc) const;

    // True only on the frame the key transitioned from up to down.
    bool IsKeyJustPressed(SDL_Scancode sc) const;

    // True only on the frame the key transitioned from down to up.
    bool IsKeyJustReleased(SDL_Scancode sc) const;

    // ── Mouse buttons ─────────────────────────────────────────────────────

    // button: 1=left, 2=middle, 3=right
    bool IsMouseDown(int button) const;
    bool IsMouseJustPressed(int button) const;
    bool IsMouseJustReleased(int button) const;

    // ── Mouse position and movement ───────────────────────────────────────

    float MouseX() const { return m_MouseX; }
    float MouseY() const { return m_MouseY; }

    // Movement since last frame in pixels.
    float MouseDeltaX() const { return m_MouseX - m_PrevMouseX; }
    float MouseDeltaY() const { return m_MouseY - m_PrevMouseY; }

    // Scroll delta this frame. Reset to 0 each BeginFrame().
    float ScrollX() const { return m_ScrollX; }
    float ScrollY() const { return m_ScrollY; }

    // Utility: convert a string name to a scancode.
    // Returns SDL_SCANCODE_UNKNOWN if the name is unrecognised.
    // Used by Lua bindings so scripts can write Input.IsKeyDown("W").
    static SDL_Scancode ScancodeFromName(const std::string& name);

private:
    static constexpr int k_MaxScancodes = SDL_SCANCODE_COUNT;
    static constexpr int k_MaxButtons   = 6; // SDL supports up to 5 + 1-based indexing

    std::array<bool, k_MaxScancodes> m_Current  {};
    std::array<bool, k_MaxScancodes> m_Previous {};

    std::array<bool, k_MaxButtons> m_MouseCurrent  {};
    std::array<bool, k_MaxButtons> m_MousePrevious {};

    float m_MouseX     = 0.f, m_MouseY     = 0.f;
    float m_PrevMouseX = 0.f, m_PrevMouseY = 0.f;
    float m_ScrollX    = 0.f, m_ScrollY    = 0.f;
};

} // namespace Cadmium
#endif // CADMIUM_INPUT_MANAGER_HPP
