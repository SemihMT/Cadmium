#ifndef CADMIUM_EDITOR_DIRTY_STATE_HPP
#define CADMIUM_EDITOR_DIRTY_STATE_HPP

namespace Cadmium::Editor
{
// NOTE: scene serialization is not implemented yet.
// saving (ctrl+S) only clears the flag for now
class EditorDirtyState
{
public:
    bool IsDirty() const { return m_Dirty; }
    void SetDirty()   { m_Dirty = true; }
    void ClearDirty() { m_Dirty = false; }

private:
    bool m_Dirty{false};
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_DIRTY_STATE_HPP
