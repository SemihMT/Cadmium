#ifndef CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
#define CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP

#include <cadmium/editor/scriptbuffer.hpp>
#include <cadmium/assets/asset_manager.hpp>
#include <cadmium/core/logger.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

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

    //  Tab management:
    // Open a script by its resolved path. If already open, switches to that tab.
    // Returns false if the file cannot be read.
    bool OpenScript(const std::string& fullPath)
    {
        // Already open? Switch to it.
        for (int i = 0; i < (int)m_Buffers.size(); ++i)
        {
            if (m_Buffers[i].GetPath() == fullPath)
            {
                SwitchToTab(i);
                return true;
            }
        }

        std::ifstream file(fullPath);
        if (!file.is_open())
        {
            Log::Error("ScriptEditor", "Cannot open '{}'", fullPath);
            return false;
        }

        std::string source(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        m_Buffers.emplace_back(fullPath, std::move(source));
        m_FocusNewTab = true;
        SwitchToTab((int)m_Buffers.size() - 1);
        return true;
    }

    void OpenBlank()
    {
        m_Buffers.emplace_back("untitled.lua");
        m_FocusNewTab = true;
        SwitchToTab((int)m_Buffers.size() - 1);
    }

    //  Run polling:
    // Call each frame. Returns true once when Run was pressed.
    // outSource is filled with the current buffer text.
    bool ConsumeRunRequest(std::string& outSource)
    {
        if (!m_RunRequested) return false;
        m_RunRequested = false;
        if (!HasActiveBuffer()) return false;
        outSource = ActiveBuffer().GetText();
        return true;
    }

    //  Error feedback
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
                    flags         = ImGuiTabItemFlags_SetSelected;
                    m_FocusNewTab = false;
                }

                bool open = true;
                if (ImGui::BeginTabItem(label.c_str(), &open, flags))
                {
                    if (m_ImGuiActiveTab != i)
                    {
                        FlushStagingBuffer();
                        m_ImGuiActiveTab = i;
                        m_ActiveTab      = i;
                        SyncStagingBuffer();
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

    void RenderToolbar()
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.f));
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

        bool canReload = HasActiveBuffer() && !ActiveBuffer().IsUnsaved();
        if (!canReload)
        {
            ImGui::BeginDisabled();
            ImGui::Button("Reload");
            ImGui::EndDisabled();
        }
        else if (ImGui::Button("Reload"))
        {
            ReloadActiveBuffer();
        }
    }


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


    void SwitchToTab(int index)
    {
        FlushStagingBuffer();
        m_ActiveTab      = index;
        m_ImGuiActiveTab = index;
        SyncStagingBuffer();
    }

    void CloseTab(int index)
    {
        m_Buffers.erase(m_Buffers.begin() + index);

        if (m_Buffers.empty())
        {
            m_ActiveTab      = -1;
            m_ImGuiActiveTab = -1;
            m_StagingBuffer[0] = '\0';
            return;
        }

        int next = std::min(index, (int)m_Buffers.size() - 1);
        SwitchToTab(next);
    }


    void SaveActiveBuffer()
    {
        if (!HasActiveBuffer()) return;

        ScriptBuffer& buf = ActiveBuffer();

        if (!buf.IsUnsaved())
        {
            WriteBufferToDisk(buf, buf.GetPath());
            return;
        }

        std::string relativePath = "scripts/" + buf.GetName();
        std::string fullPath     = m_Assets.ResolvePath(relativePath);

        std::filesystem::create_directories(
            std::filesystem::path(fullPath).parent_path());

        if (!WriteBufferToDisk(buf, fullPath))
            return;

        // Give the buffer a real path now that it exists on disk.
        buf.SetPath(fullPath);

        // Tell the asset manager a new file appeared so it shows up
        // in the asset panel without a manual refresh.
        m_Assets.Refresh();
    }

    bool WriteBufferToDisk(ScriptBuffer& buf, const std::string& fullPath)
    {
        if (fullPath.empty()) return false;

        std::ofstream file(fullPath);
        if (!file.is_open())
        {
            Log::Error("ScriptEditor", "Failed to open '{}' for writing", fullPath);
            return false;
        }

        file << buf.GetText();
        buf.MarkClean();
        Log::Info("ScriptEditor", "Saved '{}'", fullPath);
        return true;
    }

    void ReloadActiveBuffer()
    {
        if (!HasActiveBuffer()) return;

        ScriptBuffer& buf = ActiveBuffer();
        if (buf.IsUnsaved()) return;

        std::ifstream file(buf.GetPath());
        if (!file.is_open())
        {
            Log::Error("ScriptEditor", "Cannot reload '{}': file not found",
                       buf.GetPath());
            return;
        }

        std::string source(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        buf.SetText(std::move(source));
        buf.MarkClean();
        SyncStagingBuffer();
    }


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

    bool                HasActiveBuffer() const { return m_ActiveTab >= 0 && m_ActiveTab < (int)m_Buffers.size(); }
    ScriptBuffer&       ActiveBuffer()          { return m_Buffers[m_ActiveTab]; }
    const ScriptBuffer& ActiveBuffer()  const   { return m_Buffers[m_ActiveTab]; }

#endif // CADMIUM_IMGUI


    AssetManager&             m_Assets;
    std::vector<ScriptBuffer> m_Buffers;
    int                       m_ImGuiActiveTab{-1};
    int                       m_ActiveTab{-1};
    bool                      m_RunRequested{false};
    std::string               m_LastError;
    bool                      m_FocusNewTab{false};

    static constexpr size_t k_StagingCapacity = 512 * 1024;
    std::vector<char>       m_StagingBuffer;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_EDITOR_PANEL_HPP
