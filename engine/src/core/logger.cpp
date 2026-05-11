#include <cadmium/core/logger.hpp>

namespace Cadmium
{

  Logger &GetLogger()
  {
    static Logger s_Instance;
    return s_Instance;
  }

} // namespace Cadmium
