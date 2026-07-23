#include <cadmium/core/cadmium_theme.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>

namespace Cadmium
{
  namespace
  {
    ImVec4 Col(unsigned int hex, float alpha = 1.0f)
    {
      float r = ((hex >> 16) & 0xFF) / 255.0f;
      float g = ((hex >> 8)  & 0xFF) / 255.0f;
      float b = ( hex        & 0xFF) / 255.0f;
      return ImVec4(r, g, b, alpha);
    }

    constexpr unsigned int k_Charcoal      = 0x1B1A17;
    constexpr unsigned int k_Graphite      = 0x242220;
    constexpr unsigned int k_Ash           = 0x322F2B;
    constexpr unsigned int k_AshLight      = 0x3D3A35;
    constexpr unsigned int k_Bone          = 0xE8E1D4;
    constexpr unsigned int k_Umber         = 0x8C8578;
    constexpr unsigned int k_CadmiumYellow = 0xF2B705;
    constexpr unsigned int k_CadmiumOrange = 0xD9622B;
    constexpr unsigned int k_MutedRed      = 0xA83232;
  }

  void ApplyCadmiumTheme()
  {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    c[ImGuiCol_Text]                  = Col(k_Bone);
    c[ImGuiCol_TextDisabled]          = Col(k_Umber);
    c[ImGuiCol_WindowBg]              = Col(k_Charcoal);
    c[ImGuiCol_ChildBg]               = Col(k_Charcoal);
    c[ImGuiCol_PopupBg]               = Col(k_Graphite, 0.98f);
    c[ImGuiCol_Border]                = Col(k_Ash);
    c[ImGuiCol_BorderShadow]          = Col(0x000000, 0.0f);

    c[ImGuiCol_FrameBg]               = Col(k_Ash);
    c[ImGuiCol_FrameBgHovered]        = Col(k_AshLight);
    c[ImGuiCol_FrameBgActive]         = Col(k_AshLight);

    c[ImGuiCol_TitleBg]               = Col(k_Graphite);
    c[ImGuiCol_TitleBgActive]         = Col(k_Graphite);
    c[ImGuiCol_TitleBgCollapsed]      = Col(k_Graphite, 0.75f);
    c[ImGuiCol_MenuBarBg]             = Col(k_Graphite);

    c[ImGuiCol_ScrollbarBg]           = Col(k_Charcoal);
    c[ImGuiCol_ScrollbarGrab]         = Col(k_Ash);
    c[ImGuiCol_ScrollbarGrabHovered]  = Col(k_AshLight);
    c[ImGuiCol_ScrollbarGrabActive]   = Col(k_CadmiumOrange);

    c[ImGuiCol_CheckMark]             = Col(k_CadmiumYellow);
    c[ImGuiCol_SliderGrab]            = Col(k_CadmiumYellow, 0.8f);
    c[ImGuiCol_SliderGrabActive]      = Col(k_CadmiumYellow);

    c[ImGuiCol_Button]                = Col(k_Ash);
    c[ImGuiCol_ButtonHovered]         = Col(k_CadmiumOrange, 0.6f);
    c[ImGuiCol_ButtonActive]          = Col(k_CadmiumOrange);

    // (Hierarchy selection, InspectorPanel's CollapsingHeader) shares this one color.
    c[ImGuiCol_Header]                = Col(k_CadmiumYellow, 0.25f);
    c[ImGuiCol_HeaderHovered]         = Col(k_CadmiumYellow, 0.40f);
    c[ImGuiCol_HeaderActive]          = Col(k_CadmiumYellow, 0.55f);

    c[ImGuiCol_Separator]             = Col(k_Ash);
    c[ImGuiCol_SeparatorHovered]      = Col(k_CadmiumOrange, 0.6f);
    c[ImGuiCol_SeparatorActive]       = Col(k_CadmiumOrange);

    c[ImGuiCol_ResizeGrip]            = Col(k_Ash, 0.5f);
    c[ImGuiCol_ResizeGripHovered]     = Col(k_CadmiumOrange, 0.6f);
    c[ImGuiCol_ResizeGripActive]      = Col(k_CadmiumOrange);

    c[ImGuiCol_Tab]                   = Col(k_Graphite);
    c[ImGuiCol_TabHovered]            = Col(k_CadmiumYellow, 0.4f);
    c[ImGuiCol_TabActive]             = Col(k_Ash);
    c[ImGuiCol_TabUnfocused]          = Col(k_Graphite, 0.8f);
    c[ImGuiCol_TabUnfocusedActive]    = Col(k_Ash, 0.8f);

    c[ImGuiCol_DockingPreview]        = Col(k_CadmiumYellow, 0.35f);
    c[ImGuiCol_DockingEmptyBg]        = Col(k_Charcoal);

    c[ImGuiCol_PlotLines]             = Col(k_CadmiumOrange);
    c[ImGuiCol_PlotHistogram]         = Col(k_CadmiumYellow);

    c[ImGuiCol_TextSelectedBg]        = Col(k_CadmiumYellow, 0.35f);
    c[ImGuiCol_DragDropTarget]        = Col(k_CadmiumYellow, 0.9f);
    c[ImGuiCol_NavHighlight]          = Col(k_CadmiumYellow);

    // Reserved for destructive/error use only
    c[ImGuiCol_PlotLinesHovered]      = Col(k_MutedRed);

    // Shape: modest rounding and not pill-shaped
    style.WindowRounding    = 4.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 2.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 3.0f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(10.0f, 10.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
  }
}
#endif
