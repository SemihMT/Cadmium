#ifndef CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
#define CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP

#include <cadmium/editor/scriptbuffer.hpp>
#include <cadmium/assets/asset_manager.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <optional>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif


namespace Cadmium::Editor
{

class ScriptEditorPanel
{
public:
    explicit ScriptEditorPanel(AssetManager& assets)
        : m_Assets(assets)
    {
        m_StagingBuffer.resize(k_StagingCapacity, '\0');
    }

    //  Tab management

    // Open a script by handle. If already open, switches to that tab.
    // Returns false if the handle is invalid or source unavailable.
    bool OpenScript(ScriptHandle handle)
    {
        // Already open? Switch to it.
        for (int i = 0; i < (int)m_Buffers.size(); ++i)
        {
            if (m_Buffers[i].GetHandle() == handle)
            {
                m_ActiveTab = i;
                return true;
            }
        }

        const std::string* source = m_Assets.GetScriptSource(handle);
        if (!source) return false;

        // Derive display name from asset entry
        std::string name = NameFromHandle(handle);

        m_Buffers.emplace_back(handle, name, *source);
        m_ActiveTab = (int)m_Buffers.size() - 1;
        m_FocusNewTab = true;
        SyncStagingBuffer();
        return true;
    }

    void OpenBlank()
    {
        m_Buffers.emplace_back(k_InvalidHandle, "untitled.lua", "");
        m_ActiveTab = (int)m_Buffers.size() - 1;
        // Do NOT set m_ImGuiActiveTab here - ImGui hasn't confirmed it yet.
        // It will be set when BeginTabItem returns true for the new tab.
        m_FocusNewTab = true;
        SyncStagingBuffer();
    }

    //  Run polling

    // Call each frame after Render(). Returns true once when Run was pressed.
    // outSource is set to the current buffer text.
    bool ConsumeRunRequest(std::string& outSource)
    {
        if (!m_RunRequested) return false;
        m_RunRequested = false;
        if (!HasActiveBuffer()) return false;
        outSource = ActiveBuffer().GetText();
        return true;
    }

    //  Error feedback

    // Call this when the Lua runtime reports an error after a Run.
    void SetError(const std::string& error) { m_LastError = error; }
    void ClearError()                        { m_LastError.clear(); }

    //  Render

    void Render(const char* windowName = "Script Editor")
    {
#ifdef CADMIUM_IMGUI
        ImGui::Begin(windowName);

        if (m_Buffers.empty())
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

    //  Empty state

    void RenderEmptyState()
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float  textW = ImGui::CalcTextSize("No script open").x;

        ImGui::SetCursorPosX((avail.x - textW) * 0.5f);
        ImGui::SetCursorPosY(avail.y * 0.4f);
        ImGui::TextDisabled("No script open");

        float btnW = 120.f;
        ImGui::SetCursorPosX((avail.x - btnW) * 0.5f);
        if (ImGui::Button("New Script", { btnW, 0 }))
            OpenBlank();
    }

    //  Tabs
    void RenderTabs()
    {
        if (ImGui::BeginTabBar("##scripts"))
        {
            for (int i = 0; i < (int)m_Buffers.size(); ++i)
            {
                std::string label = m_Buffers[i].GetName();
                if (m_Buffers[i].IsDirty())
                    label += " *";
                label += "##" + std::to_string(i);

                ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
                if (m_FocusNewTab && i == (int)m_Buffers.size() - 1)
                {
                    flags = ImGuiTabItemFlags_SetSelected;
                    m_FocusNewTab = false;
                }

                bool open = true;
                if (ImGui::BeginTabItem(label.c_str(), &open, flags))
                {
                    // ImGui says this tab is active.
                    // Only sync if it actually changed.
                    if (m_ImGuiActiveTab != i)
                    {
                        FlushStagingBuffer(); // save old tab content
                        m_ActiveTab = i;
                        m_ImGuiActiveTab = i;
                        SyncStagingBuffer(); // load new tab content
                    }
                    ImGui::EndTabItem();
                }

                if (!open)
                {
                    CloseTab(i);
                    break;
                }
            }

            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
                OpenBlank();

            ImGui::EndTabBar();
        }
    }

    //  Toolbar

    void RenderToolbar()
    {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.2f, 0.6f, 0.2f, 1.f));
        if (ImGui::Button("Run ▶"))
        {
            FlushStagingBuffer();
            m_RunRequested = true;
            m_LastError.clear();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        bool canSave = HasActiveBuffer() && ActiveBuffer().IsDirty();
        if (!canSave)
        {
            ImGui::BeginDisabled();
            ImGui::Button("Save");
            ImGui::EndDisabled();
        }
        else if (ImGui::Button("Save"))
        {
            SaveActiveBuffer();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reload"))
            ReloadActiveBuffer();
    }

    //  Text area

    void RenderTextArea()
    {
        float statusBarH = ImGui::GetFrameHeightWithSpacing() + 4.f;
        ImVec2 textSize = ImGui::GetContentRegionAvail();
        textSize.y -= statusBarH;
        if (textSize.y < 0.f)
            textSize.y = 0.f;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

        std::string widgetId = "##source_" + std::to_string(m_ActiveTab);

        bool changed = ImGui::InputTextMultiline(
            widgetId.c_str(),
            m_StagingBuffer.data(),
            m_StagingBuffer.size(),
            textSize,
            ImGuiInputTextFlags_AllowTabInput);

        ImGui::PopStyleVar();

        if (changed && HasActiveBuffer())
            ActiveBuffer().SetText(m_StagingBuffer.data());
    }

    //  Status bar

    void RenderStatusBar()
    {
        ImGui::Separator();

        if (!m_LastError.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
            ImGui::TextUnformatted(m_LastError.c_str());
            ImGui::PopStyleColor();
        }
        else if (HasActiveBuffer() && ActiveBuffer().IsDirty())
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
    }

    //  Tab operations

    void CloseTab(int index)
    {
        m_Buffers.erase(m_Buffers.begin() + index);

        if (m_Buffers.empty())
        {
            m_ActiveTab = -1;
            m_ImGuiActiveTab = -1;
            m_StagingBuffer[0] = '\0';
            return;
        }

        m_ActiveTab = std::min(index, (int)m_Buffers.size() - 1);
        m_ImGuiActiveTab = m_ActiveTab;
        SyncStagingBuffer();
    }

    //  Save / Reload

    void SaveActiveBuffer()
    {
        if (!HasActiveBuffer())
            return;

        ScriptBuffer &buf = ActiveBuffer();

        // Known path - existing asset
        if (buf.GetHandle() != k_InvalidHandle)
        {
            SaveBufferToPath(buf, FindPathForHandle(buf.GetHandle()));
            return;
        }

        // Untitled buffer - save as new file in scripts directory
        std::string relativePath = "scripts/" + buf.GetName();
        std::string fullPath = m_Assets.ResolvePath(relativePath);

        // Ensure directory exists
        std::filesystem::create_directories(
            std::filesystem::path(fullPath).parent_path());

        if (!SaveBufferToPath(buf, fullPath))
            return;

        // Register with asset manager so it gets a real handle
        ScriptHandle handle = m_Assets.LoadScript(relativePath);
        if (handle != k_InvalidHandle)
        {
            // Replace buffer with a named one
            std::string name = buf.GetName();
            std::string text = buf.GetText();
            m_Buffers[m_ActiveTab] = ScriptBuffer(handle, name, text);
            m_Buffers[m_ActiveTab].MarkClean();
        }
        else
        {
            Cadmium::Log::Error("ScriptEditor", "Error loading script from {}", relativePath);
        }

    }
    bool SaveBufferToPath(ScriptBuffer &buf, const std::string &fullPath)
    {
        if (fullPath.empty())
            return false;

        std::ofstream file(fullPath);
        if (!file.is_open())
        {
            SDL_Log("[ScriptEditor] Failed to open '%s' for writing",
                    fullPath.c_str());
            return false;
        }

        file << buf.GetText();
        buf.MarkClean();
        SDL_Log("[ScriptEditor] Saved '%s'", fullPath.c_str());
        return true;
    }
    void ReloadActiveBuffer()
    {
        if (!HasActiveBuffer()) return;

        ScriptBuffer& buf = ActiveBuffer();
        ScriptHandle  handle = buf.GetHandle();

        if (handle == k_InvalidHandle) return; // untitled, nothing to reload

        // Force reload from disk via asset manager
        m_Assets.UnloadScript(handle);
        ScriptHandle fresh = m_Assets.LoadScript(FindPathForHandle(handle));

        if (fresh == k_InvalidHandle) return;

        const std::string* source = m_Assets.GetScriptSource(fresh);
        if (!source) return;

        buf.SetText(*source);
        buf.MarkClean();
        SyncStagingBuffer();
    }

    //  Staging buffer helpers
    // ImGui::InputTextMultiline needs a mutable C buffer.
    // We sync to/from ScriptBuffer on tab switch and on edit.

    void SyncStagingBuffer()
    {
        if (!HasActiveBuffer())
        {
            m_StagingBuffer[0] = '\0';
            return;
        }

        const std::string& text = ActiveBuffer().GetText();
        size_t len = std::min(text.size(), k_StagingCapacity - 1);
        std::memcpy(m_StagingBuffer.data(), text.data(), len);
        m_StagingBuffer[len] = '\0';
    }

    void FlushStagingBuffer()
    {
        if (!HasActiveBuffer()) return;
        ActiveBuffer().SetText(m_StagingBuffer.data());
    }

    //  Utilities

    bool          HasActiveBuffer() const { return m_ActiveTab >= 0 && m_ActiveTab < (int)m_Buffers.size(); }
    ScriptBuffer& ActiveBuffer()          { return m_Buffers[m_ActiveTab]; }
    const ScriptBuffer& ActiveBuffer() const { return m_Buffers[m_ActiveTab]; }

    std::string NameFromHandle(ScriptHandle handle) const
    {
        for (const auto& entry : m_Assets.GetAllEntries())
            if (entry.handle == handle)
                return entry.filename;
        return "unknown.lua";
    }

    std::string FindPathForHandle(ScriptHandle handle) const
    {
        for (const auto &entry : m_Assets.GetAllEntries())
            if (entry.handle == handle)
                return m_Assets.ResolvePath(entry.path);
        return {};
    }
#endif // CADMIUM_IMGUI

    //  Data

    AssetManager&            m_Assets;
    std::vector<ScriptBuffer> m_Buffers;
    int                      m_ImGuiActiveTab{-1};
    int                      m_ActiveTab{-1};
    bool                     m_RunRequested{false};
    std::string              m_LastError;
    bool                     m_FocusNewTab{false};

    static constexpr size_t k_StagingCapacity = 512 * 1024;
    std::vector<char> m_StagingBuffer;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
