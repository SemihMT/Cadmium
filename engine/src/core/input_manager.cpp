#include <cadmium/core/input_manager.hpp>
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>

namespace Cadmium
{

void InputManager::BeginFrame()
{
    // Shift current → previous for delta computation
    m_Previous      = m_Current;
    m_MousePrevious = m_MouseCurrent;
    m_PrevMouseX    = m_MouseX;
    m_PrevMouseY    = m_MouseY;

    m_ScrollX = 0.f;
    m_ScrollY = 0.f;
}

void InputManager::SnapshotPost()
{
    // SDL_GetKeyboardState returns a pointer to SDL's internal state array.
    // It is valid until the next SDL_PumpEvents / SDL_PollEvent call.
    // We snapshot it immediately after polling so it reflects this frame.
    int numKeys = 0;
    const bool* sdlKeys = SDL_GetKeyboardState(&numKeys);

    int count = std::min(numKeys, k_MaxScancodes);
    for (int i = 0; i < count; ++i)
        m_Current[i] = sdlKeys[i];

    // Mouse buttons
    float mx, my;
    Uint32 buttons = SDL_GetMouseState(&mx, &my);
    m_MouseX = mx;
    m_MouseY = my;

    // SDL uses 1-based button indices. We store them 1-based (index 0 unused).
    m_MouseCurrent[1] = (buttons & SDL_BUTTON_LMASK)  != 0;
    m_MouseCurrent[2] = (buttons & SDL_BUTTON_MMASK)  != 0;
    m_MouseCurrent[3] = (buttons & SDL_BUTTON_RMASK)  != 0;
    m_MouseCurrent[4] = (buttons & SDL_BUTTON_X1MASK) != 0;
    m_MouseCurrent[5] = (buttons & SDL_BUTTON_X2MASK) != 0;
}

void InputManager::OnMouseWheel(float x, float y)
{
    m_ScrollX += x;
    m_ScrollY += y;
}

// ── Keyboard ──────────────────────────────────────────────────────────────

bool InputManager::IsKeyDown(SDL_Scancode sc) const
{
    if (sc < 0 || sc >= k_MaxScancodes) return false;
    return m_Current[sc];
}

bool InputManager::IsKeyJustPressed(SDL_Scancode sc) const
{
    if (sc < 0 || sc >= k_MaxScancodes) return false;
    return m_Current[sc] && !m_Previous[sc];
}

bool InputManager::IsKeyJustReleased(SDL_Scancode sc) const
{
    if (sc < 0 || sc >= k_MaxScancodes) return false;
    return !m_Current[sc] && m_Previous[sc];
}

// ── Mouse ─────────────────────────────────────────────────────────────────

bool InputManager::IsMouseDown(int button) const
{
    if (button < 1 || button >= k_MaxButtons) return false;
    return m_MouseCurrent[button];
}

bool InputManager::IsMouseJustPressed(int button) const
{
    if (button < 1 || button >= k_MaxButtons) return false;
    return m_MouseCurrent[button] && !m_MousePrevious[button];
}

bool InputManager::IsMouseJustReleased(int button) const
{
    if (button < 1 || button >= k_MaxButtons) return false;
    return !m_MouseCurrent[button] && m_MousePrevious[button];
}

// ── Scancode name lookup ──────────────────────────────────────────────────

SDL_Scancode InputManager::ScancodeFromName(const std::string& name)
{
    SDL_Scancode sc = SDL_GetScancodeFromName(name.c_str());
    return sc; // Returns SDL_SCANCODE_UNKNOWN if not found
}

} // namespace Cadmium
