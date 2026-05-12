#ifndef CADMIUM_ASSETS_ASSET_TYPES_HPP
#define CADMIUM_ASSETS_ASSET_TYPES_HPP

#include <cstdint>
#include <string>

  namespace Cadmium
  {
    //  Handle types
    // Opaque integer handles. 0 is always invalid.
    // Scripts store and pass these around - they never inspect the value.

    using TextureHandle = uint32_t;
    using FontHandle = uint32_t;
    using SoundHandle = uint32_t; // reserved for audio phase

    static constexpr uint32_t k_InvalidHandle = 0;

    //  Asset metadata
    // Stored alongside each loaded asset. Used by the asset panel.

    enum class AssetType
    {
      Unknown,
      Texture,
      Font,
      Sound,
      Script,
      // expandable - Tilemap, Shader, etc.
    };

    // Experiment: use C++26 to retrieve type names using reflection
    inline const char *AssetTypeName(AssetType type)
    {
      switch (type)
      {
      case AssetType::Texture:
        return "Texture";
      case AssetType::Font:
        return "Font";
      case AssetType::Sound:
        return "Sound";
      case AssetType::Script:
        return "Script";
      default:
        return "Unknown";
      }
    }

    inline AssetType AssetTypeFromExtension(const std::string &ext)
    {
      if (ext == ".png" || ext == ".jpg" ||
          ext == ".jpeg" || ext == ".bmp" || ext == ".webp")
        return AssetType::Texture;

      if (ext == ".ttf" || ext == ".otf")
        return AssetType::Font;

      if (ext == ".wav" || ext == ".ogg" || ext == ".mp3")
        return AssetType::Sound;

      if (ext == ".lua")
        return AssetType::Script;

      return AssetType::Unknown;
    }

    struct AssetEntry
    {
      uint32_t handle = k_InvalidHandle;
      AssetType type = AssetType::Unknown;
      std::string path;     // relative to project root
      std::string filename; // just the filename for display
      bool loaded = false;

      // For textures - populated after load for display in asset panel
      int width = 0;
      int height = 0;

      // For fonts - the size it was loaded at
      int fontSize = 0;
    };

  } // namespace Cadmium

#endif // CADMIUM_ASSETS_ASSET_TYPES_HPP
