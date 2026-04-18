#include <cadmium/core/assets.hpp>
#include <SDL3/SDL.h>

namespace Cadmium
{

  std::string AssetPath(const std::string &relative)
  {
#ifdef CADMIUM_PLATFORM_WEB
    return relative;
#else
    const char *base = SDL_GetBasePath();
    return std::string(base) + relative;
#endif
  }

} // namespace Cadmium
