#ifndef CADMIUM_ASSETS_ASSET_TYPES_HPP
#define CADMIUM_ASSETS_ASSET_TYPES_HPP

#include <cadmium/core/handles.hpp>
#include <string>

  namespace Cadmium
  {
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
      inline const char* AssetTypeName(AssetType type)
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
      AssetHandle handle = k_InvalidAsset;
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
