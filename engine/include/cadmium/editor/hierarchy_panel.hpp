#ifndef CADMIUM_EDITOR_HIERARCHY_PANEL
#define CADMIUM_EDITOR_HIERARCHY_PANEL
#include <cadmium/ecs/world.hpp>
#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif
namespace Cadmium::Editor
{
    class HierarchyPanel
    {
      public:
        void Render(World& world, const char* title = "Hierarchy")
        {
#ifdef CADMIUM_IMGUI
            ImGui::Begin(title);

            for (Entity e : world.AllEntities())
                if (!world.HasComponent<Parent>(e)) // roots only at the top level
                    DrawEntityNode(world, e);

            ImGui::End();
#endif
        }

        void DrawEntityNode(World& world, Entity e)
        {
            std::string label = "Entity " + std::to_string(EntityIndex(e));
            if (auto* tag = world.TryGetComponent<Tag>(e))
                label = tag->name + "##" + std::to_string(e);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (e == m_Selected)
                flags |= ImGuiTreeNodeFlags_Selected;

            bool open = ImGui::TreeNodeEx(label.c_str(), flags);
            if (ImGui::IsItemClicked())
                m_Selected = e;

            if (open)
            {
                for (Entity child : world.AllEntities())
                    if (auto* p = world.TryGetComponent<Parent>(child))
                        if (p->entity == e)
                            DrawEntityNode(world, child);
                ImGui::TreePop();
            }
        }
        Entity GetSelected() const { return m_Selected; }

      private:
        Entity m_Selected{k_NullEntity};
    };
} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_HIERARCHY_PANEL
