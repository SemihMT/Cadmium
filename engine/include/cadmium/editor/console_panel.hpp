#ifndef CADMIUM_EDITOR_CONSOLE_PANEL_HPP
#define CADMIUM_EDITOR_CONSOLE_PANEL_HPP

#include <cadmium/core/logger.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

#include <deque>
#include <mutex>
#include <string>
#include <array>

namespace Cadmium::Editor
{

class ConsolePanel
{
public:
    //  Sink management
    // Call Attach() when the owning layer attaches, Detach() when it detaches.
    // This keeps the sink lifetime tied to the panel's visibility.

    void Attach()
    {
        m_SinkId = GetLogger().AddSink([this](const LogRecord& record)
        {
            PushRecord(record);
        });
    }

    void Detach()
    {
        GetLogger().RemoveSink(m_SinkId);
        m_SinkId = 0;
    }

    //  Render

    void Render(const char* windowName = "Console")
    {
#ifdef CADMIUM_IMGUI
        ImGui::Begin(windowName);
        RenderToolbar();
        RenderOutput();
        ImGui::End();
#endif
    }

private:
#ifdef CADMIUM_IMGUI

    //  Toolbar

    void RenderToolbar()
    {
        if (ImGui::Button("Clear"))
        {
            std::lock_guard lock(m_Mutex);
            m_Records.clear();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.f);
        ImGui::Combo("##level", &m_FilterLevel, k_LevelNames.data(), (int)k_LevelNames.size());

        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        ImGui::InputTextWithHint("##cat", "Filter category...",
                                  m_CategoryFilter, sizeof(m_CategoryFilter));

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

        ImGui::Separator();
    }

    //  Output

    void RenderOutput()
    {
        ImGui::BeginChild("##output", ImVec2(0, 0), false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        std::lock_guard lock(m_Mutex);

        std::string catFilter(m_CategoryFilter);

        for (const auto& record : m_Records)
        {
            // Level filter
            if ((int)record.level < m_FilterLevel)
                continue;

            // Category filter
            if (!catFilter.empty() &&
                record.category.find(catFilter) == std::string::npos)
                continue;

            // Color by level
            ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(record.level));

            // [LEVEL][Category] message
            std::string line = std::format("[{}][{}] {}",
                LogLevelName(record.level),
                record.category,
                record.message);

            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }

        // Auto-scroll to bottom when new records arrive
        if (m_AutoScroll && m_ScrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
            m_ScrollToBottom = false;
        }

        ImGui::EndChild();
    }

    //  Helpers

    static ImVec4 LevelColor(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Trace: return { 0.6f, 0.6f, 0.6f, 1.f }; // grey
            case LogLevel::Debug: return { 0.4f, 0.8f, 0.9f, 1.f }; // cyan
            case LogLevel::Info:  return { 0.8f, 0.8f, 0.8f, 1.f }; // white
            case LogLevel::Warn:  return { 1.0f, 0.8f, 0.2f, 1.f }; // yellow
            case LogLevel::Error: return { 1.0f, 0.4f, 0.4f, 1.f }; // red
            case LogLevel::Fatal: return { 1.0f, 0.2f, 0.8f, 1.f }; // magenta
            default:              return { 1.0f, 1.0f, 1.0f, 1.f };
        }
    }

#endif // CADMIUM_IMGUI

    //  Record storage

    void PushRecord(const LogRecord& record)
    {
        std::lock_guard lock(m_Mutex);

        if (m_Records.size() >= k_MaxRecords)
            m_Records.pop_front();

        m_Records.push_back(record);
        m_ScrollToBottom = true;
    }

    //  Data

    static constexpr size_t k_MaxRecords = 1000;

    static constexpr std::array<const char*, 6> k_LevelNames = {
        "Trace", "Debug", "Info", "Warn", "Error", "Fatal"
    };

    std::deque<LogRecord> m_Records;
    std::mutex            m_Mutex;       // PushRecord called from any thread
    uint32_t              m_SinkId{0};

    int  m_FilterLevel{0};              // index into k_LevelNames
    char m_CategoryFilter[64]{};
    bool m_AutoScroll{true};
    bool m_ScrollToBottom{false};
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_CONSOLE_PANEL_HPP
