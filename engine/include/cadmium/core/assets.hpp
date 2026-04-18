#ifndef CADMIUM_ASSETS_HPP
#define CADMIUM_ASSETS_HPP
#include <string>

namespace Cadmium
{

  /// Returns the absolute path to an asset file.
  /// On native: prepends SDL_GetBasePath() so paths resolve relative to the executable.
  /// On Emscripten: returns the path as-is since the virtual FS mirrors the same structure.
  std::string AssetPath(const std::string &relative);

} // namespace Cadmium
#endif // CADMIUM_ASSETS_HPP
