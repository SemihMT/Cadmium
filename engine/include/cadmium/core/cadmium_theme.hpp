#ifndef CADMIUM_CADMIUM_THEME
#define CADMIUM_CADMIUM_THEME

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Cadmium
{
    void ApplyCadmiumTheme();

    namespace Theme
    {
        constexpr unsigned int k_Ultramarine = 0x2A4B8D;
        constexpr unsigned int k_Viridian = 0x2F6E51;
        constexpr unsigned int k_ManganeseViolet = 0x6B3F69;
        constexpr unsigned int k_RawSienna = 0x9C5A2E;
        constexpr unsigned int k_Unknown = 0x3D3A35; // AshLight

#ifdef CADMIUM_IMGUI
        inline ImU32 BadgeColor(unsigned int hex, unsigned char alpha = 220)
        {
            return IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, alpha);
        }
#endif
    } // namespace Theme
} // namespace Cadmium
#endif
