#include "hud_layer.hpp"
#include <cmath>
#include <cstring>

namespace Sandbox
{
  // 7-segment display layout per digit
  // Segments: top, top-left, top-right, middle, bot-left, bot-right, bottom
  static constexpr bool k_Segments[10][7] = {
    { true,  true,  true,  false, true,  true,  true  }, // 0
    { false, false, true,  false, false, true,  false }, // 1
    { true,  false, true,  true,  true,  false, true  }, // 2
    { true,  false, true,  true,  false, true,  true  }, // 3
    { false, true,  true,  true,  false, true,  false }, // 4
    { true,  true,  false, true,  false, true,  true  }, // 5
    { true,  true,  false, true,  true,  true,  true  }, // 6
    { true,  false, true,  false, false, true,  false }, // 7
    { true,  true,  true,  true,  true,  true,  true  }, // 8
    { true,  true,  true,  true,  false, true,  true  }, // 9
  };

  void HUDLayer::OnAttach()
  {
    m_ScoreToken = Subscribe<ScoreChangedEvent>([this](const ScoreChangedEvent& e)
    {
      m_DisplayScore = e.score;
    });

    m_WaveToken = Subscribe<WaveCompletedEvent>([this](const WaveCompletedEvent& e)
    {
      m_Wave = e.wave;
    });

    m_DeathToken = Subscribe<PlayerDiedEvent>([this](const PlayerDiedEvent&)
    {
    });
  }

  void HUDLayer::OnDetach()
  {
    m_ScoreToken = {};
    m_WaveToken  = {};
    m_DeathToken = {};
  }

  void HUDLayer::OnEvent(SDL_Event& event)
  {
    if (event.type == SDL_EVENT_KEY_DOWN)
      if (event.key.key == SDLK_R && m_State->gameOver)
        Post(RestartEvent{});
  }

  void HUDLayer::OnRender(SDL_Renderer* renderer)
  {
    DrawScore(renderer);
    DrawWave(renderer);

    if (m_State->gameOver)
      DrawGameOver(renderer);
  }

  void HUDLayer::DrawScore(SDL_Renderer* renderer) const
  {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    DrawNumber(renderer, m_DisplayScore, 20.0f, 20.0f, 2.0f);
  }

  void HUDLayer::DrawWave(SDL_Renderer* renderer) const
  {
    SDL_SetRenderDrawColor(renderer, 180, 180, 255, 255);
    float x = static_cast<float>(GetWidth()) - 80.0f;
    DrawText(renderer, "WAVE", x, 20.0f, 1.5f);
    DrawNumber(renderer, m_Wave, x + 10.0f, 40.0f, 2.0f);
  }

  void HUDLayer::DrawGameOver(SDL_Renderer* renderer) const
  {
    float cx = static_cast<float>(GetWidth())  * 0.5f;
    float cy = static_cast<float>(GetHeight()) * 0.5f;

    SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
    DrawText(renderer, "GAME OVER", cx - 80.0f, cy - 30.0f, 2.5f);

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    DrawText(renderer, "PRESS R TO RESTART", cx - 90.0f, cy + 20.0f, 1.5f);

    SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
    DrawText(renderer, "SCORE", cx - 40.0f, cy + 60.0f, 1.5f);
    DrawNumber(renderer, m_DisplayScore, cx - 20.0f, cy + 80.0f, 2.0f);
  }

  void HUDLayer::DrawSegment(SDL_Renderer* renderer,
                              float x, float y,
                              float scale,
                              bool horizontal,
                              float offX, float offY) const
  {
    float px = x + offX * scale;
    float py = y + offY * scale;

    if (horizontal)
      SDL_RenderLine(renderer, px, py, px + 6.0f * scale, py);
    else
      SDL_RenderLine(renderer, px, py, px, py + 8.0f * scale);
  }

  void HUDLayer::DrawDigit(SDL_Renderer* renderer,
                            int digit,
                            float x, float y,
                            float scale) const
  {
    if (digit < 0 || digit > 9) return;
    const bool* segs = k_Segments[digit];

    // top
    if (segs[0]) DrawSegment(renderer, x, y, scale, true,  0.0f,  0.0f);
    // top-left
    if (segs[1]) DrawSegment(renderer, x, y, scale, false, 0.0f,  0.0f);
    // top-right
    if (segs[2]) DrawSegment(renderer, x, y, scale, false, 6.0f,  0.0f);
    // middle
    if (segs[3]) DrawSegment(renderer, x, y, scale, true,  0.0f,  8.0f);
    // bot-left
    if (segs[4]) DrawSegment(renderer, x, y, scale, false, 0.0f,  8.0f);
    // bot-right
    if (segs[5]) DrawSegment(renderer, x, y, scale, false, 6.0f,  8.0f);
    // bottom
    if (segs[6]) DrawSegment(renderer, x, y, scale, true,  0.0f, 16.0f);
  }

  void HUDLayer::DrawNumber(SDL_Renderer* renderer,
                             int number,
                             float x, float y,
                             float scale) const
  {
    std::string str = std::to_string(number);
    float digitWidth = 8.0f * scale;

    for (size_t i = 0; i < str.size(); i++)
      DrawDigit(renderer, str[i] - '0',
                x + static_cast<float>(i) * digitWidth, y, scale);
  }

  void HUDLayer::DrawText(SDL_Renderer* renderer,
                           const char* text,
                           float x, float y,
                           float scale) const
  {
    // Minimal 3x5 pixel font for capital letters and space
    // Each letter is a bitmask of a 3-wide by 5-tall grid
    static constexpr uint16_t k_Font[27] = {
      0b010101111101101, // A
      0b110101110101110, // B
      0b011100100100011, // C
      0b110101101101110, // D
      0b111100110100111, // E
      0b111100110100100, // F
      0b011100101101011, // G
      0b101101111101101, // H
      0b111010010010111, // I
      0b001001001101010, // J
      0b101101110101101, // K
      0b100100100100111, // L
      0b101111111101101, // M
      0b101101111101101, // N
      0b010101101101010, // O
      0b110101110100100, // P
      0b010101101111011, // Q
      0b110101110101101, // R
      0b011100010001110, // S
      0b111010010010010, // T
      0b101101101101011, // U
      0b101101101101010, // V
      0b101101111111101, // W
      0b101101010101101, // X
      0b101101010010010, // Y
      0b111001010100111, // Z
      0b000000000000000, // space
    };

    float charWidth = 5.0f * scale;

    for (size_t i = 0; i < strlen(text); i++)
    {
      char c = text[i];
      int  idx = (c >= 'A' && c <= 'Z') ? c - 'A'
               : (c == ' ')             ? 26
               : -1;

      if (idx < 0) { continue; }

      uint16_t glyph = k_Font[idx];
      for (int row = 0; row < 5; row++)
      {
        for (int col = 0; col < 3; col++)
        {
          int bit = 14 - (row * 3 + col);
          if (!((glyph >> bit) & 1)) continue;

          float px = x + static_cast<float>(i) * charWidth
                       + static_cast<float>(col) * scale;
          float py = y + static_cast<float>(row) * scale;
          SDL_RenderLine(renderer, px, py, px + scale, py);
        }
      }
    }
  }

} // namespace Sandbox
