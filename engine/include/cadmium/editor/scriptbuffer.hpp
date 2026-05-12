#ifndef CADMIUM_EDITOR_SCRIPT_BUFFER_HPP
#define CADMIUM_EDITOR_SCRIPT_BUFFER_HPP

#include <cadmium/assets/asset_types.hpp>
#include <string>

namespace Cadmium::Editor
{

// Mutable editor copy of a script asset's source text.
// The editor owns one ScriptBuffer per open tab.
// AssetManager holds the immutable disk copy - ScriptBuffer holds what
// the user is currently editing, which may differ.
class ScriptBuffer
{
public:
    ScriptBuffer(ScriptHandle handle,
                 std::string  name,
                 std::string  source)
        : m_Handle{handle}
        , m_Name{std::move(name)}
        , m_Text{std::move(source)}
    {}

    //  Identity
    ScriptHandle       GetHandle() const { return m_Handle; }
    const std::string& GetName()   const { return m_Name;   }

    //  Text access
    // Non-const overload is intentionally absent from GetText.
    // All mutations go through SetText so the dirty flag is always correct.
    const std::string& GetText() const { return m_Text; }

    void SetText(std::string text)
    {
        if (text == m_Text) return; // avoid spurious dirty on no-op
        m_Text  = std::move(text);
        m_Dirty = true;
    }

    //  Dirty tracking
    bool IsDirty()   const { return m_Dirty; }
    void MarkClean()       { m_Dirty = false; }

private:
    ScriptHandle m_Handle;
    std::string  m_Name;
    std::string  m_Text;
    bool         m_Dirty{false};
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_BUFFER_HPP
