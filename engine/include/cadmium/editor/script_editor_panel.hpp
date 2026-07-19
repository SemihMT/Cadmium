#ifndef CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
#define CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP

#include <cadmium/assets/asset_manager.hpp>
#include <cadmium/core/logger.hpp>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <TextEditor.h>

#ifdef CADMIUM_IMGUI
#   include <imgui.h>
#   include <imgui_internal.h>
#endif

namespace Cadmium::Editor
{

class ScriptEditorPanel
{
public:
    explicit ScriptEditorPanel(AssetManager& assets)
        : m_Assets(assets)
    {}


    bool OpenScript(const std::string& fullPath)
    {
#ifdef CADMIUM_IMGUI
        for (int i = 0; i < TabCount(); ++i)
        {
            if (m_Tabs[i].path == fullPath)
            {
                SwitchToTab(i);
                return true;
            }
        }

        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open())
        {
            Log::Error("ScriptEditor", "Cannot open '{}'", fullPath);
            return false;
        }

        std::string source((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        PushTab(std::filesystem::path(fullPath).filename().string(), fullPath, source);
        return true;
#else
        return false;
#endif
    }

    void OpenBlank()
    {
#ifdef CADMIUM_IMGUI
        PushTab("untitled.lua", "", "");
#endif
    }


    bool ConsumeRunRequest(std::string& outSource)
    {
#ifdef CADMIUM_IMGUI
        if (!m_RunRequested) return false;
        m_RunRequested = false;
        if (!HasActive()) return false;
        outSource = Active().editor.GetText();
        return true;
#else
        return false;
#endif
    }


    void SetError(const std::string& error, int line = -1)
    {
#ifdef CADMIUM_IMGUI
        m_LastError = error;
        if (!HasActive()) return;
        Active().editor.ClearMarkers();
        if (line >= 0)
        {
            Active().editor.AddMarker(
                line,
                IM_COL32(220, 60,  60, 255),
                IM_COL32(255, 80,  80,  40),
                "Error",
                error.c_str());
        }
#endif
    }

    void ClearError()
    {
#ifdef CADMIUM_IMGUI
        m_LastError.clear();
        if (HasActive())
            Active().editor.ClearMarkers();
#endif
    }

    void Render(const char* windowName = "Script Editor")
    {
#ifdef CADMIUM_IMGUI
        ImGui::Begin(windowName);

        if (m_Tabs.empty())
        {
            RenderEmptyState();
            ImGui::End();
            return;
        }

        RenderTabs();
        RenderToolbar();
        RenderTextArea();
        RenderStatusBar();

        ImGui::End();
#endif
    }

private:
#ifdef CADMIUM_IMGUI


    struct Tab
    {
        uint64_t    id   = 0;
        std::string name;       // display name
        std::string path;       // empty when unsaved
        TextEditor  editor;
        bool        dirty = false;

        bool isUnsaved() const { return path.empty(); }
    };

    int  TabCount() const  { return static_cast<int>(m_Tabs.size()); }
    bool HasActive() const { return m_ActiveTab >= 0 && m_ActiveTab < TabCount(); }
    Tab& Active()          { return m_Tabs[m_ActiveTab]; }

    void ApplyEditorSettings(TextEditor& ed)
    {
        ed.SetPalette(m_CurrentTheme == 1
                      ? TextEditor::GetLightPalette()
                      : TextEditor::GetDarkPalette());

        ed.SetShowWhitespacesEnabled(m_ShowWhitespaces);
    }

    void PushTab(const std::string& name,
                 const std::string& path,
                 const std::string& source)
    {
        m_Tabs.emplace_back();
        Tab& tab  = m_Tabs.back();
        tab.id    = ++m_NextTabId;
        tab.name  = name;
        tab.path  = path;
        tab.dirty = false;

        tab.editor.SetLanguage(TextEditor::Language::Lua());
        tab.editor.SetShowLineNumbersEnabled(true);
        tab.editor.SetShowScrollbarMiniMapEnabled(true);
        tab.editor.SetShowMatchingBrackets(true);
        tab.editor.SetAutoIndentEnabled(true);
        tab.editor.SetTabSize(4);
        tab.editor.SetText(source);
        ApplyEditorSettings(tab.editor);

        bool* dirtyPtr = &tab.dirty;
        tab.editor.SetChangeCallback([dirtyPtr]() { *dirtyPtr = true; });

        m_FocusNewTab = true;
        SwitchToTab(TabCount() - 1);
    }

    void SwitchToTab(int index)
    {
        if (index >= 0 && index < TabCount())
            m_ActiveTab = index;
    }

    void CloseTab(int index)
    {
        if (index < 0 || index >= TabCount()) return;
        m_Tabs.erase(m_Tabs.begin() + index);

        if (m_Tabs.empty()) { m_ActiveTab = -1; return; }

        if (m_ActiveTab >= index)
            m_ActiveTab = std::max(0, m_ActiveTab - 1);
    }


    void RenderEmptyState()
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize("No script open").x) * 0.5f);
        ImGui::SetCursorPosY(avail.y * 0.4f);
        ImGui::TextDisabled("No script open");

        constexpr float btnW = 120.f;
        ImGui::SetCursorPosX((avail.x - btnW) * 0.5f);
        if (ImGui::Button("New Script", {btnW, 0}))
            OpenBlank();
    }

    void RenderTabs()
    {
        if (!ImGui::BeginTabBar("##scripts")) return;

        for (int i = 0; i < TabCount(); ++i)
        {

            std::string label = m_Tabs[i].name;
            if (m_Tabs[i].dirty) label += " *";
            label += "##" + std::to_string(m_Tabs[i].id);

            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            if (m_FocusNewTab && i == TabCount() - 1)
            {
                flags         = ImGuiTabItemFlags_SetSelected;
                m_FocusNewTab = false;
            }

            bool open = true;
            if (ImGui::BeginTabItem(label.c_str(), &open, flags))
            {
                m_ActiveTab = i;
                ImGui::EndTabItem();
            }

            if (!open) { CloseTab(i); break; }
        }

        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            OpenBlank();

        ImGui::EndTabBar();
    }

    void RenderToolbar()
    {
        if (!HasActive()) return;
        TextEditor& ed = Active().editor;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.f));
        if (ImGui::Button("Run \xE2\x96\xB6")) { m_RunRequested = true; ClearError(); }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        {
            bool canSave = Active().dirty;
            if (!canSave) ImGui::BeginDisabled();
            if (ImGui::Button("Save")) SaveActive();
            if (!canSave) ImGui::EndDisabled();
        }
        ImGui::SameLine();

        {
            bool canReload = !Active().isUnsaved() && !Active().dirty;
            if (!canReload) ImGui::BeginDisabled();
            if (ImGui::Button("Reload")) ReloadActive();
            if (!canReload) ImGui::EndDisabled();
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            bool can = ed.CanUndo();
            if (!can) ImGui::BeginDisabled();
            if (ImGui::Button("Undo")) ed.Undo();
            if (!can) ImGui::EndDisabled();
        }
        ImGui::SameLine();
        {
            bool can = ed.CanRedo();
            if (!can) ImGui::BeginDisabled();
            if (ImGui::Button("Redo")) ed.Redo();
            if (!can) ImGui::EndDisabled();
        }
        ImGui::SameLine();

        if (ImGui::Button("Copy"))  ed.Copy();
        ImGui::SameLine();
        if (ImGui::Button("Paste")) ed.Paste();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        if (ImGui::Button("Find"))
            ed.OpenFindReplaceWindow();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(80.f);
        const char* themes[] = { "Dark", "Light" };
        if (ImGui::Combo("##Theme", &m_CurrentTheme, themes, 2))
        {
            for (auto& tab : m_Tabs)
                ApplyEditorSettings(tab.editor);
        }

        ImGui::SameLine();
        if (ImGui::Checkbox("Whitespace", &m_ShowWhitespaces))
        {
            for (auto& tab : m_Tabs)
                tab.editor.SetShowWhitespacesEnabled(m_ShowWhitespaces);
        }
    }

    void RenderTextArea()
    {
        if (!HasActive()) return;

        const float statusBarH = ImGui::GetFrameHeightWithSpacing() + 4.f;
        ImVec2 size            = ImGui::GetContentRegionAvail();
        size.y -= statusBarH;
        if (size.y < 0.f) size.y = 0.f;


        std::string id = "##editor_" + std::to_string(Active().id);
        Active().editor.Render(id.c_str(), size, false);
    }

    void RenderStatusBar()
    {
        ImGui::Separator();

        if (!m_LastError.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
            ImGui::TextUnformatted(m_LastError.c_str());
            ImGui::PopStyleColor();
        }
        else if (HasActive() && Active().dirty)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.4f, 1.f));
            ImGui::TextUnformatted("Modified");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.5f, 1.f));
            ImGui::TextUnformatted("Ready");
            ImGui::PopStyleColor();
        }

        if (HasActive())
        {
            auto pos = Active().editor.GetMainCursorPosition();
            char buf[40];
            snprintf(buf, sizeof(buf), "Ln %d  Col %d", pos.line + 1, pos.column + 1);
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(buf).x - 8.f);
            ImGui::TextDisabled("%s", buf);
        }
    }

    void SaveActive()
    {
        if (!HasActive()) return;
        Tab& tab = Active();

        if (!tab.isUnsaved())
        {
            WriteToDisk(tab, tab.path);
            return;
        }

        std::string fullPath = m_Assets.ResolvePath("scripts/" + tab.name);
        std::filesystem::create_directories(
            std::filesystem::path(fullPath).parent_path());

        tab.path = fullPath;
        if (!WriteToDisk(tab, fullPath))
            tab.path.clear();
        else
            m_Assets.Refresh();
    }

    bool WriteToDisk(Tab& tab, const std::string& fullPath)
    {
        if (fullPath.empty()) return false;

        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open())
        {
            Log::Error("ScriptEditor", "Failed to open '{}' for writing", fullPath);
            return false;
        }

        file << tab.editor.GetText();
        tab.dirty = false;
        Log::Info("ScriptEditor", "Saved '{}'", fullPath);
        return true;
    }

    void ReloadActive()
    {
        if (!HasActive()) return;
        Tab& tab = Active();
        if (tab.isUnsaved()) return;

        std::ifstream file(tab.path, std::ios::binary);
        if (!file.is_open())
        {
            Log::Error("ScriptEditor", "Cannot reload '{}': not found", tab.path);
            return;
        }

        std::string source((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        tab.editor.SetText(source);
        tab.dirty = false;
        ClearError();
    }

#endif // CADMIUM_IMGUI
    AssetManager& m_Assets;
#ifdef CADMIUM_IMGUI
    // std::deque: push_back never relocates existing elements, so the
    // &tab.dirty pointer captured in each SetChangeCallback is always valid.
    std::deque<Tab> m_Tabs;
    int             m_ActiveTab     {-1};
    bool            m_RunRequested  {false};
    std::string     m_LastError;
    bool            m_FocusNewTab   {false};
    uint64_t        m_NextTabId     {0};
    int             m_CurrentTheme  {0};          // 0 = Dark, 1 = Light
    bool            m_ShowWhitespaces{false};
#endif
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
