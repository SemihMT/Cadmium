#ifndef CADMIUM_EDITOR_EDITOR_STATE_HPP
#define CADMIUM_EDITOR_EDITOR_STATE_HPP

namespace Cadmium::Editor
{

enum class EditorState
{
    Edit,   // game paused, panels interactive, script editor active
    Play,   // game running, panels minimized
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_EDITOR_STATE_HPP
