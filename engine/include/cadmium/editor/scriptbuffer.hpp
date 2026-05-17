#ifndef CADMIUM_EDITOR_SCRIPT_BUFFER_HPP
#define CADMIUM_EDITOR_SCRIPT_BUFFER_HPP

#include <filesystem>
#include <string>

namespace Cadmium::Editor
{

// Mutable editor copy of a script's source text.
// The editor owns one ScriptBuffer per open tab.
// Identity is the file path, not an asset handle.
// An empty path means the buffer has never been saved to disk.
class ScriptBuffer
{
public:
    // Construct from a file on disk (path is the full resolved path).
    ScriptBuffer(std::string path, std::string source)
        : m_Path{std::move(path)}
        , m_Name{std::filesystem::path(m_Path).filename().string()}
        , m_Text{std::move(source)}
    {}

    // Construct an untitled buffer with no backing file.
    explicit ScriptBuffer(std::string name)
        : m_Path{}
        , m_Name{std::move(name)}
        , m_Text{}
    {}

    //  Identity

    // Full resolved path on disk. Empty if unsaved.
    const std::string& GetPath() const { return m_Path; }
    const std::string& GetName() const { return m_Name; }

    bool IsUnsaved() const { return m_Path.empty(); }

    // Called after a successful "Save As" to give the buffer a real path.
    void SetPath(std::string path)
    {
        m_Path = std::move(path);
        m_Name = std::filesystem::path(m_Path).filename().string();
    }


    const std::string& GetText() const { return m_Text; }

    void SetText(std::string text)
    {
        if (text == m_Text) return;
        m_Text  = std::move(text);
        m_Dirty = true;
    }

    bool IsDirty()   const { return m_Dirty; }
    void MarkClean()       { m_Dirty = false; }

private:
    std::string m_Path;   // full resolved path, or empty
    std::string m_Name;   // filename only, for display
    std::string m_Text;
    bool        m_Dirty{false};
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_BUFFER_HPP
