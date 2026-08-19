#ifndef CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
#define CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP

#include <cadmium/assets/asset_manager.hpp>
#include <cadmium/core/logger.hpp>
#include <TextEditor.h>
#include <imgui.h>

#include <deque>
#include <fstream>
#include <sstream>
#include <string>

namespace Cadmium::Editor
{
    class ScriptEditorPanel
    {
      public:
        explicit ScriptEditorPanel(AssetManager& assets) : m_Assets(assets) {}

        // Open or focus a script for editing.
        void OpenScript(const std::string& relativePath)
        {
            for (size_t i = 0; i < m_Tabs.size(); ++i)
            {
                if (m_Tabs[i].relativePath == relativePath)
                {
                    m_FocusTabIndex = static_cast<int>(i);
                    m_WindowOpen = true;
                    return;
                }
            }

            std::string absolutePath = m_Assets.ResolvePath(relativePath);
            std::ifstream file(absolutePath, std::ios::binary);
            if (!file)
            {
                Log::Warn("ScriptEditorPanel", "Could not open '{}' for reading.", absolutePath);
                return;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();

            Tab tab;
            tab.relativePath = relativePath;
            size_t slash = relativePath.find_last_of("/\\");
            tab.filename = slash == std::string::npos ? relativePath : relativePath.substr(slash + 1);
            tab.editor.SetLanguage(TextEditor::Language::Lua());
            tab.editor.SetText(buffer.str());

            std::string path = relativePath;
            tab.editor.SetChangeCallback([this, path]() { MarkDirty(path); });

            m_Tabs.push_back(std::move(tab));
            m_FocusTabIndex = static_cast<int>(m_Tabs.size()) - 1;
            m_WindowOpen = true;
        }

        [[nodiscard]] bool HasOpenScripts() const { return !m_Tabs.empty(); }

        void Render(const char* windowName = "Script Editor", bool* open = nullptr)
        {
            if (open ? !*open : !m_WindowOpen)
                return;

            if (!ImGui::Begin(windowName, open ? open : &m_WindowOpen))
            {
                ImGui::End();
                return;
            }

            if (m_Tabs.empty())
            {
                ImGui::TextDisabled("No scripts open.");
                ImGui::TextDisabled("Double-click a .lua file in the Assets panel to edit it.");
                ImGui::End();
                return;
            }

            if (ImGui::BeginTabBar("##script_tabs", ImGuiTabBarFlags_Reorderable))
            {
                for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
                    RenderTab(i);

                RenderUnsavedChangesPopup();

                ImGui::EndTabBar();
            }

            ImGui::End();
        }

      private:
        struct Tab
        {
            std::string relativePath;
            std::string filename;
            TextEditor editor;
            bool dirty{false};
        };

        void MarkDirty(const std::string& relativePath)
        {
            for (Tab& tab : m_Tabs)
            {
                if (tab.relativePath == relativePath)
                {
                    tab.dirty = true;
                    return;
                }
            }
        }

        void RenderTab(int index)
        {
            Tab& tab = m_Tabs[index];

            ImGuiTabItemFlags flags = tab.dirty ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None;
            if (m_FocusTabIndex == index)
            {
                flags |= ImGuiTabItemFlags_SetSelected;
                m_FocusTabIndex = -1;
            }

            bool tabStillOpen = true;
            ImGui::PushID(tab.relativePath.c_str());
            bool visible = ImGui::BeginTabItem(tab.filename.c_str(), &tabStillOpen, flags);

            if (visible)
            {
                RenderToolbar(tab);
                ImVec2 avail = ImGui::GetContentRegionAvail();
                tab.editor.Render("##editor", avail);
                ImGui::EndTabItem();
            }
            ImGui::PopID();

            if (!tabStillOpen)
            {
                if (tab.dirty)
                {
                    m_PendingClose = index;
                    ImGui::OpenPopup("Unsaved Changes##script_editor");
                }
                else
                {
                    m_Tabs.erase(m_Tabs.begin() + index);
                }
            }
        }

        void RenderToolbar(Tab& tab)
        {
            bool ctrlS = ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false);
            if (ImGui::Button("Save") || (tab.dirty && ctrlS))
                Save(tab);

            ImGui::SameLine();
            ImGui::TextDisabled("%s", tab.relativePath.c_str());

            if (tab.dirty)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.f), "(unsaved)");
            }

            ImGui::Separator();
        }

        void Save(Tab& tab)
        {
            std::string absolutePath = m_Assets.ResolvePath(tab.relativePath);
            std::ofstream file(absolutePath, std::ios::binary | std::ios::trunc);
            if (!file)
            {
                Log::Error("ScriptEditorPanel", "Could not save '{}'.", absolutePath);
                return;
            }
            file << tab.editor.GetText();
            tab.dirty = false;
        }

        void RenderUnsavedChangesPopup()
        {
            if (!ImGui::BeginPopupModal(
                    "Unsaved Changes##script_editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                return;

            if (m_PendingClose >= 0 && m_PendingClose < static_cast<int>(m_Tabs.size()))
            {
                ImGui::Text("Save changes to '%s' before closing?",
                            m_Tabs[m_PendingClose].filename.c_str());
                ImGui::Separator();

                if (ImGui::Button("Save"))
                {
                    Save(m_Tabs[m_PendingClose]);
                    m_Tabs.erase(m_Tabs.begin() + m_PendingClose);
                    m_PendingClose = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Discard"))
                {
                    m_Tabs.erase(m_Tabs.begin() + m_PendingClose);
                    m_PendingClose = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    m_PendingClose = -1;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

        AssetManager& m_Assets;
        std::deque<Tab> m_Tabs;
        int m_FocusTabIndex{-1};
        int m_PendingClose{-1};
        bool m_WindowOpen{true};
    };

} // namespace Cadmium::Editor
#endif // CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
