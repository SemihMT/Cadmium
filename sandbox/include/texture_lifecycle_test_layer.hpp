#ifndef CADMIUM_TEXTURE_LIFECYCLE_TEST_LAYER_HPP
#define CADMIUM_TEXTURE_LIFECYCLE_TEST_LAYER_HPP

// Manual/visual smoke test for the textured sprite pipeline: texture
// lifecycle (create/destroy), atlas sub-rects, flip/rotation, and
// draw-order preservation between the flat and textured pipelines.

// Controls (see OnEvent):
//   SPACE - advance to the next test phase
//   D     - destroy the loaded texture immediately (out of sequence),
//           to confirm nothing crashes for the rest of the run


#include "cadmium/scripting/script_system.hpp"
#include <cadmium/core/layer.hpp>
#include <cadmium/core/logger.hpp>
#include <cadmium/render/renderer.hpp>
#include <cadmium/core/draw_command_queue.hpp>
#include <cadmium/core/handles.hpp>
#include <cadmium/ecs/world.hpp>

#include <SDL3/SDL.h>

#include <string>

namespace Cadmium
{
    class TextureLifecycleTestLayer : public Layer
    {
      public:
        explicit TextureLifecycleTestLayer(std::string atlasPath)
            : Layer("TextureLifecycleTest"), m_AtlasPath{std::move(atlasPath)}
        {
        }

        void OnAttach() override
        {
            SetDefaultBackground(true);
            GetWorld().RegisterSystem<Cadmium::ScriptSystem>(0);

            m_Texture = GetAssets().LoadTexture(m_AtlasPath);
            TextureDesc desc = GetRenderer().GetTextureDesc(m_Texture);

            bool loadOk = m_Texture != k_InvalidTexture && desc.width > 0 && desc.height > 0;
            LogResult("Load texture + desc sanity", loadOk,
                      std::format("handle={} desc=({}x{})", m_Texture, desc.width, desc.height));

            m_SecondTexture = GetAssets().LoadTexture(m_AtlasPath);
            LogResult("Second LoadTexture call returns a valid (possibly distinct) handle",
                      m_SecondTexture != k_InvalidTexture,
                      std::format("handle={}", m_SecondTexture));

            Log::Info("TextureLifecycleTest",
                      "SPACE = next phase, D = destroy texture early. Starting at phase 0.");
        }

        void OnEvent(SDL_Event& event) override
        {
            if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat)
                return;

            if (event.key.key == SDLK_SPACE)
            {
                m_Phase = (m_Phase + 1) % k_PhaseCount;
                Log::Info("TextureLifecycleTest", "-> phase {}: {}", m_Phase, PhaseName(m_Phase));
            }
            else if (event.key.key == SDLK_D && !m_DestroyedEarly)
            {
                m_DestroyedEarly = true;
                GetRenderer().DestroyTexture(m_Texture);

                TextureDesc after = GetRenderer().GetTextureDesc(m_Texture);
                LogResult("GetTextureDesc after DestroyTexture returns {0,0}",
                          after.width == 0 && after.height == 0,
                          std::format("desc=({}x{})", after.width, after.height));

                Log::Info("TextureLifecycleTest",
                          "Texture destroyed early. Sprites using it should now silently stop "
                          "drawing on both backends - if anything crashes or a stale image "
                          "keeps appearing, that's a FAIL.");
            }
        }

        void OnRender(SDL_Renderer*) override
        {
            IRenderer& renderer = GetRenderer();

            switch (m_Phase)
            {
                case 0: RenderOrderPhase(renderer); break;
                case 1: RenderAtlasPhase(renderer); break;
                case 2: RenderFlipRotationPhase(renderer); break;
                case 3: RenderMultiTextureInterleavePhase(renderer); break;
                case 4: RenderFullscreenPhase(renderer); break;
                default: break;
            }

            // Runs regardless of phase/destroy state to make sure an
            // invalid handle never crashes either backend.
            DrawCmd::Sprite bogus{};
            bogus.textureHandle = 0xDEADBEEF;
            bogus.x = 40.f;
            bogus.y = static_cast<float>(GetHeight()) - 40.f;
            bogus.color = Color{1.f, 1.f, 1.f, 1.f};
            renderer.DrawSprite(bogus);
        }

      private:
        static constexpr int k_PhaseCount = 5;

        static const char* PhaseName(int phase)
        {
            switch (phase)
            {
                case 0: return "draw-order preservation (flat <-> textured interleave)";
                case 1: return "atlas sub-rects";
                case 2: return "flip + rotation";
                case 3: return "multi-texture interleave (bind group switching)";
                case 4: return "fullscreen texture (bypasses camera)";
                default: return "?";
            }
        }

        void LogResult(const std::string& what, bool pass, const std::string& detail)
        {
            if (pass)
                Log::Info("TextureLifecycleTest", "PASS - {} ({})", what, detail);
            else
                Log::Error("TextureLifecycleTest", "FAIL - {} ({})", what, detail);
        }

        // Phase 0: the actual regression test for the run-list fix.
        // Pair A: rect drawn, THEN an opaque sprite on top -> sprite
        //   should fully occlude the rect. This still looks right even
        //   under the old bug (flat-always-first), so it's a baseline
        //   sanity check, not the real test.
        // Pair B: sprite drawn, THEN an opaque rect on top -> rect
        //   should fully occlude the sprite. Under the old bug (all
        //   flat vertices flushed before any textured ones, regardless
        //   of call order) the sprite would incorrectly win and show
        //   through. THIS is the case that catches the regression.
        void RenderOrderPhase(IRenderer& renderer)
        {
            if (m_Texture == k_InvalidTexture)
                return;

            float boxSize = 64.f;

            // Pair A - rect then sprite (baseline, expected: sprite wins, always did)
            {
                DrawCmd::Rect rect{};
                rect.x = 0.f;
                rect.y = 0.f;
                rect.w = boxSize;
                rect.h = boxSize;
                rect.filled = true;
                rect.color = Color{1.f, 0.f, 1.f, 1.f}; // magenta
                renderer.DrawRect(rect);

                DrawCmd::Sprite sprite{};
                sprite.textureHandle = m_Texture;
                sprite.x = 0.f;
                sprite.y = 0.f;
                sprite.w = boxSize;
                sprite.h = boxSize;
                sprite.color = Color{1.f, 1.f, 1.f, 1.f};
                renderer.DrawSprite(sprite);
            }

            // Pair B - sprite then rect (the actual regression check:
            // expected result is a SOLID magenta box with no sprite
            // visible at all; any sprite pixels showing through == FAIL)
            {
                DrawCmd::Sprite sprite{};
                sprite.textureHandle = m_Texture;
                sprite.x = 120.f;
                sprite.y = 0.f;
                sprite.w = boxSize;
                sprite.h = boxSize;
                sprite.color = Color{1.f, 1.f, 1.f, 1.f};
                renderer.DrawSprite(sprite);

                DrawCmd::Rect rect{};
                rect.x = 120.f;
                rect.y = 0.f;
                rect.w = boxSize;
                rect.h = boxSize;
                rect.filled = true;
                rect.color = Color{1.f, 0.f, 1.f, 1.f}; // opaque magenta, must fully cover
                renderer.DrawRect(rect);
            }

            if (!m_LoggedOrderPhaseHint)
            {
                m_LoggedOrderPhaseHint = true;
                Log::Info("TextureLifecycleTest",
                          "Phase 0: left box (rect->sprite) should show the sprite. Right box "
                          "(sprite->rect) should be a SOLID magenta square with NO sprite "
                          "visible - if you see the sprite peeking through on the right, "
                          "draw-order between the flat and textured pipelines regressed.");
            }
        }

        // Phase 1: sub-rects of the same texture drawn side by side,
        // with w/h left unset so natural size should equal the
        // sub-rect size, not the whole sheet.
        void RenderAtlasPhase(IRenderer& renderer)
        {
            if (m_Texture == k_InvalidTexture)
                return;

            TextureDesc desc = renderer.GetTextureDesc(m_Texture);
            if (desc.width <= 0 || desc.height <= 0)
                return; // destroyed - nothing to show, this is expected after phase D

            float halfW = static_cast<float>(desc.width) * 0.5f;
            float halfH = static_cast<float>(desc.height) * 0.5f;

            struct Quadrant { float x, y, w, h; };
            Quadrant quads[4] = {
                {0.f,    0.f,    halfW, halfH}, // top-left
                {halfW,  0.f,    halfW, halfH}, // top-right
                {0.f,    halfH,  halfW, halfH}, // bottom-left
                {halfW,  halfH,  halfW, halfH}, // bottom-right
            };

            float startX = 0.f, y = 0.f, spacing = 90.f;
            for (int i = 0; i < 4; ++i)
            {
                DrawCmd::Sprite sprite{};
                sprite.textureHandle = m_Texture;
                sprite.x = startX + spacing * static_cast<float>(i);
                sprite.y = y;
                // w/h intentionally left at 0 - natural size must come
                // from the source rect (quads[i].w/h), not the full
                // texture. If these render at the full sheet's size,
                // the natural-size fallback fix regressed.
                sprite.srcX = quads[i].x;
                sprite.srcY = quads[i].y;
                sprite.srcW = quads[i].w;
                sprite.srcH = quads[i].h;
                sprite.color = Color{1.f, 1.f, 1.f, 1.f};
                renderer.DrawSprite(sprite);
            }

            if (!m_LoggedAtlasPhaseHint)
            {
                m_LoggedAtlasPhaseHint = true;
                Log::Info("TextureLifecycleTest",
                          "Phase 1: four quadrants of the source image, each should render at "
                          "roughly half the atlas's width/height (not the full sheet size), "
                          "and each should show a visually distinct region.");
            }
        }

        void RenderFlipRotationPhase(IRenderer& renderer)
        {
            if (m_Texture == k_InvalidTexture)
                return;

            float y = 0.f, startX = 0.f, spacing = 90.f, size = 64.f;

            auto drawOne = [&](float x, bool flipX, bool flipY, float rotationDeg)
            {
                DrawCmd::Sprite sprite{};
                sprite.textureHandle = m_Texture;
                sprite.x = x;
                sprite.y = y;
                sprite.w = size;
                sprite.h = size;
                sprite.flipX = flipX;
                sprite.flipY = flipY;
                sprite.rotation = rotationDeg;
                sprite.color = Color{1.f, 1.f, 1.f, 1.f};
                renderer.DrawSprite(sprite);
            };

            drawOne(startX + spacing * 0, false, false, 0.f);
            drawOne(startX + spacing * 1, true,  false, 0.f);
            drawOne(startX + spacing * 2, false, true,  0.f);
            drawOne(startX + spacing * 3, false, false, 45.f);
            drawOne(startX + spacing * 4, false, false, 90.f);

            if (!m_LoggedFlipPhaseHint)
            {
                m_LoggedFlipPhaseHint = true;
                Log::Info("TextureLifecycleTest",
                          "Phase 2, left to right: normal, flipX, flipY, rotated 45deg, "
                          "rotated 90deg. SDL2D and WebGPU should look identical here.");
            }
        }

        // Phase 3: alternate between two texture handles rapidly to
        // confirm bind-group switching happens per run and the batch
        // merge logic doesn't skip a texture change between two
        // non-adjacent runs of the same handle.
        void RenderMultiTextureInterleavePhase(IRenderer& renderer)
        {
            if (m_Texture == k_InvalidTexture || m_SecondTexture == k_InvalidTexture)
                return;

            float y = 0.f, startX = 0.f, spacing = 70.f, size = 56.f;
            TextureHandle sequence[6] = {m_Texture, m_SecondTexture, m_Texture,
                                          m_Texture, m_SecondTexture, m_SecondTexture};

            for (int i = 0; i < 6; ++i)
            {
                DrawCmd::Sprite sprite{};
                sprite.textureHandle = sequence[i];
                sprite.x = startX + spacing * static_cast<float>(i);
                sprite.y = y;
                sprite.w = size;
                sprite.h = size;
                sprite.color = Color{1.f, 1.f, 1.f, 1.f};
                renderer.DrawSprite(sprite);
            }

            if (!m_LoggedMultiTexPhaseHint)
            {
                m_LoggedMultiTexPhaseHint = true;
                Log::Info("TextureLifecycleTest",
                          "Phase 3: six sprites alternating between two texture handles "
                          "(A B A A B B). All six should render identically regardless of "
                          "handle - if any are blank or show the wrong image, a run got "
                          "merged across a texture change incorrectly.");
            }
        }

        void RenderFullscreenPhase(IRenderer& renderer)
        {
            if (m_Texture == k_InvalidTexture)
                return;

            renderer.DrawFullscreenTexture(m_Texture);

            if (!m_LoggedFullscreenPhaseHint)
            {
                m_LoggedFullscreenPhaseHint = true;
                Log::Info("TextureLifecycleTest",
                          "Phase 4: texture stretched fullscreen. Move the camera (if your "
                          "test scene supports it) - this should NOT move or scale, since "
                          "DrawFullscreenTexture bypasses the camera transform.");
            }
        }

        std::string m_AtlasPath;
        TextureHandle m_Texture{k_InvalidTexture};
        TextureHandle m_SecondTexture{k_InvalidTexture};
        int m_Phase{0};
        bool m_DestroyedEarly{false};

        bool m_LoggedOrderPhaseHint{false};
        bool m_LoggedAtlasPhaseHint{false};
        bool m_LoggedFlipPhaseHint{false};
        bool m_LoggedMultiTexPhaseHint{false};
        bool m_LoggedFullscreenPhaseHint{false};
    };
} // namespace Cadmium

#endif // CADMIUM_TEXTURE_LIFECYCLE_TEST_LAYER_HPP
