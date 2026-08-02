#ifndef CADMIUM_HANDLES_HPP
#define CADMIUM_HANDLES_HPP

#include <cstdint>

namespace Cadmium
{

    using AssetHandle = uint32_t; // Generic asset handle without type. Used for AssetEntry struct in asset_types.hpp
    using TextureHandle = uint32_t;
    using FontHandle = uint32_t;
    using SoundHandle = uint32_t;
    using ScriptHandle = uint32_t;

    inline constexpr AssetHandle k_InvalidAsset = 0;
    inline constexpr TextureHandle k_InvalidTexture = 0;
    inline constexpr FontHandle k_InvalidFont = 0;
    inline constexpr SoundHandle k_InvalidSound = 0;
    inline constexpr ScriptHandle k_InvalidScript = 0;

} // namespace Cadmium

#endif
