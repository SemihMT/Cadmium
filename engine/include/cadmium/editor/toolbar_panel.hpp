#ifndef CADMIUM_EDITOR_TOOLBAR_PANEL_HPP
#define CADMIUM_EDITOR_TOOLBAR_PANEL_HPP

#include <cadmium/editor/editor_state.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Cadmium::Editor
{

// Renders the Play/Edit toolbar.
// Owns nothing — reads and writes state via the reference passed in.
// EditorOverlayLayer owns the state and passes it here.
class ToolbarPanel
{
public:
    // Render the toolbar as a full-width menu-bar-style strip.
    // state is read and written directly.
    // Returns true if state changed this frame.
    bool Render(EditorState& state)
    {
#ifdef CADMIUM_IMGUI
        bool changed = false;

        // Full-width overlay toolbar pinned to the top of the screen.
        ImGuiIO&  io      = ImGui::GetIO();
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar     |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::SetNextWindowPos({ 0.f, 0.f });
        ImGui::SetNextWindowSize({ io.DisplaySize.x, k_ToolbarHeight });
        ImGui::SetNextWindowBgAlpha(0.85f);

        ImGui::Begin("##toolbar", nullptr, flags);

        //  State indicator

        ImVec4 stateColor = (state == EditorState::Play)
            ? ImVec4(0.2f, 0.9f, 0.2f, 1.f)   // green when playing
            : ImVec4(0.9f, 0.7f, 0.2f, 1.f);  // amber when editing

        ImGui::PushStyleColor(ImGuiCol_Text, stateColor);
        ImGui::Text("  %-6s", StateLabel(state));
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        //  Buttons

        if (state == EditorState::Edit)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.2f, 0.6f, 0.2f, 1.f));
            if (ImGui::Button("  ▶  Play  "))
            {
                state   = EditorState::Play;
                changed = true;
            }
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.7f, 0.2f, 0.2f, 1.f));
            if (ImGui::Button("  ■  Stop  "))
            {
                state   = EditorState::Edit;
                changed = true;
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        //  FPS display

        ImGui::TextDisabled("%.1f fps", io.Framerate);

        ImGui::End();

        return changed;
#else
        return false;
#endif
    }

    // Height in pixels - used by EditorOverlayLayer to offset other panels
    // so they don't render under the toolbar.
    static constexpr float k_ToolbarHeight = 32.f;

private:
    static const char* StateLabel(EditorState state)
    {
        switch (state)
        {
            case EditorState::Edit: return "EDIT";
            case EditorState::Play: return "PLAY";
            default:                return "?";
        }
    }
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_TOOLBAR_PANEL_HPP
