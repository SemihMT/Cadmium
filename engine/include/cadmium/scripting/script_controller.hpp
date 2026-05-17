#ifndef CADMIUM_SCRIPTING_SCRIPT_CONTROLLER_HPP
#define CADMIUM_SCRIPTING_SCRIPT_CONTROLLER_HPP

#include <string>

namespace Cadmium {

struct IScriptController {
    virtual ~IScriptController() = default;
    virtual bool Reload(const std::string& source) = 0;
    virtual void Pause(bool paused)                 = 0;
};

} // namespace Cadmium
#endif // CADMIUM_SCRIPTING_SCRIPT_CONTROLLER_HPP
