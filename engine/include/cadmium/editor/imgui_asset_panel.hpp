#ifndef CADMIUM_EDITOR_IMGUI_ASSET_PANEL_HPP
#define CADMIUM_EDITOR_IMGUI_ASSET_PANEL_HPP

#include <cadmium/assets/asset_manager.hpp>
#include <imgui.h>
#include <string>
#include <functional>

namespace Cadmium::Editor
{

//  AssetPanel
// ImGui panel that displays all project assets organised by type.
// Shows texture thumbnails, file metadata, and load status.
// Calls an optional callback when an asset is double-clicked -
// the editor uses this to open scripts, insert asset references etc.
class AssetPanel
{
public:
    // Callback signature: (relativePath, assetType)
    using SelectCallback = std::function<void(const std::string&, AssetType)>;

    explicit AssetPanel(AssetManager& assets)
        : m_Assets(assets)
    {}

    void SetOnSelect(SelectCallback cb) { m_OnSelect = std::move(cb); }

    // Call from your ImGui render pass each frame.
    // windowName: the ImGui window title
    void Render(const char* windowName = "Assets")
    {
        ImGui::Begin(windowName);

        //  Toolbar
        if (ImGui::Button("Refresh"))
            m_Assets.Refresh();

        ImGui::SameLine();

        // View mode toggle
        ImGui::Text("View:");
        ImGui::SameLine();
        if (ImGui::RadioButton("List", m_ViewMode == ViewMode::List))
            m_ViewMode = ViewMode::List;
        ImGui::SameLine();
        if (ImGui::RadioButton("Grid", m_ViewMode == ViewMode::Grid))
            m_ViewMode = ViewMode::Grid;

        // Thumbnail size slider (grid mode only)
        if (m_ViewMode == ViewMode::Grid)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.f);
            ImGui::SliderInt("Size", &m_ThumbnailSize, 48, 256);
        }

        ImGui::Separator();

        //  Filter bar
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##filter", "Filter assets...",
                                  m_FilterBuf, sizeof(m_FilterBuf));
        std::string filter(m_FilterBuf);

        ImGui::Spacing();

        //  Asset list
        const auto& entries = m_Assets.GetAllEntries();

        if (entries.empty())
        {
            ImGui::TextDisabled("No assets found.");
            ImGui::TextDisabled("Set a project root to scan for files.");
            ImGui::End();
            return;
        }

        // Group by type
        const AssetType typeOrder[] = {
            AssetType::Texture,
            AssetType::Script,
            AssetType::Font,
            AssetType::Sound,
            AssetType::Unknown
        };

        for (AssetType type : typeOrder)
        {
            // Collect entries of this type that match the filter
            std::vector<const AssetEntry*> filtered;
            for (const auto& entry : entries)
            {
                if (entry.type != type) continue;
                if (!filter.empty())
                {
                    // Case-insensitive substring match on filename
                    std::string lower = entry.filename;
                    std::string lowerFilter = filter;
                    std::transform(lower.begin(), lower.end(),
                                   lower.begin(), ::tolower);
                    std::transform(lowerFilter.begin(), lowerFilter.end(),
                                   lowerFilter.begin(), ::tolower);
                    if (lower.find(lowerFilter) == std::string::npos)
                        continue;
                }
                filtered.push_back(&entry);
            }

            if (filtered.empty()) continue;

            // Section header
            bool open = ImGui::CollapsingHeader(
                AssetTypeName(type),
                ImGuiTreeNodeFlags_DefaultOpen);

            if (!open) continue;

            ImGui::PushID(AssetTypeName(type));

            if (m_ViewMode == ViewMode::List)
                RenderList(filtered);
            else
                RenderGrid(filtered);

            ImGui::PopID();
            ImGui::Spacing();
        }

        ImGui::End();
    }

private:
    enum class ViewMode { List, Grid };

    AssetManager&  m_Assets;
    SelectCallback m_OnSelect;
    ViewMode       m_ViewMode    = ViewMode::Grid;
    int            m_ThumbnailSize = 96;
    char           m_FilterBuf[128] {};
    std::string    m_Selected;   // currently selected path

    //  List view
    void RenderList(const std::vector<const AssetEntry*>& entries)
    {
        for (const AssetEntry* entry : entries)
        {
            bool isSelected = (m_Selected == entry->path);

            // Status indicator
            ImVec4 statusColor = entry->loaded
                ? ImVec4(0.3f, 0.9f, 0.3f, 1.f)   // green = loaded
                : ImVec4(0.5f, 0.5f, 0.5f, 1.f);   // gray = on disk only

            ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
            ImGui::TextUnformatted("●");
            ImGui::PopStyleColor();
            ImGui::SameLine();

            // Selectable row
            ImGui::PushID(entry->path.c_str());
            if (ImGui::Selectable(entry->filename.c_str(),
                                   isSelected,
                                   ImGuiSelectableFlags_AllowDoubleClick))
            {
                m_Selected = entry->path;
                if (ImGui::IsMouseDoubleClicked(0) && m_OnSelect)
                    m_OnSelect(entry->path, entry->type);
            }

            // Tooltip with metadata
            if (ImGui::IsItemHovered())
                RenderTooltip(*entry);

            // Right-click context menu
            RenderContextMenu(*entry);

            ImGui::PopID();
        }
    }

    //  Grid view
    void RenderGrid(const std::vector<const AssetEntry *> &entries)
    {
        float thumbF = (float)m_ThumbnailSize;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float textH = ImGui::GetTextLineHeightWithSpacing();

        // Full width of one grid item
        float cellSize = thumbF + spacing;

        float panelW = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, (int)(panelW / cellSize));

        int i = 0;

        for (const AssetEntry *entry : entries)
        {
            ImGui::PushID(entry->path.c_str());

            bool isSelected = (m_Selected == entry->path);

            // Entire cell region
            ImVec2 cellStart = ImGui::GetCursorScreenPos();

            // Selection background
            if (isSelected)
            {
                ImGui::GetWindowDrawList()->AddRectFilled(
                    cellStart,
                    ImVec2(
                        cellStart.x + thumbF,
                        cellStart.y + thumbF + textH + 6.f),
                    IM_COL32(60, 100, 180, 120),
                    4.f);
            }

            //
            // THUMBNAIL REGION
            //

            ImGui::BeginGroup();

            if (entry->type == AssetType::Texture && entry->loaded)
            {
                SDL_Texture *tex = m_Assets.GetTexture(entry->handle);

                if (tex)
                {
                    float aspect = (entry->height > 0)
                                       ? (float)entry->width / (float)entry->height
                                       : 1.f;

                    float dispW = thumbF;
                    float dispH = thumbF;

                    if (aspect > 1.f)
                        dispH = thumbF / aspect;
                    else
                        dispW = thumbF * aspect;

                    float padX = (thumbF - dispW) * 0.5f;
                    float padY = (thumbF - dispH) * 0.5f;

                    ImVec2 cursor = ImGui::GetCursorScreenPos();

                    // Reserve exact thumbnail space
                    ImGui::InvisibleButton("thumb", ImVec2(thumbF, thumbF));

                    // Draw centered texture manually
                    ImGui::GetWindowDrawList()->AddImage(
                        (ImTextureID)(intptr_t)tex,
                        ImVec2(cursor.x + padX, cursor.y + padY),
                        ImVec2(cursor.x + padX + dispW,
                               cursor.y + padY + dispH));
                }
                else
                {
                    RenderPlaceholder(thumbF, entry->type);
                }
            }
            else
            {
                RenderPlaceholder(thumbF, entry->type);
            }

            //
            // INTERACTION
            //

            if (ImGui::IsItemClicked())
            {
                m_Selected = entry->path;

                if (ImGui::IsMouseDoubleClicked(0) && m_OnSelect)
                    m_OnSelect(entry->path, entry->type);
            }

            if (ImGui::IsItemHovered())
                RenderTooltip(*entry);

            RenderContextMenu(*entry);

            //
            // LABEL
            //

            std::string label =
                TruncateFilename(entry->filename, (int)(thumbF / 7.f));

            float labelW =
                ImGui::CalcTextSize(label.c_str()).x;

            float labelPad =
                std::max(0.f, (thumbF - labelW) * 0.5f);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + labelPad);

            ImGui::TextUnformatted(label.c_str());

            ImGui::EndGroup();

            ImGui::PopID();

            //
            // GRID WRAPPING
            //

            ++i;

            if (i % columns != 0)
                ImGui::SameLine();
        }
    }

    //  Type placeholder
    void RenderPlaceholder(float size, AssetType type)
    {
        ImVec2 pos  = ImGui::GetCursorScreenPos();
        ImVec2 end  = ImVec2(pos.x + size, pos.y + size);
        auto* dl    = ImGui::GetWindowDrawList();

        // Background
        ImU32 bgColor;
        const char* label;
        switch (type)
        {
            case AssetType::Texture:
                bgColor = IM_COL32(20,  100,  200,  200); label = "TEX"; break;
            case AssetType::Script:
                bgColor = IM_COL32(40,  80,  40,  200); label = "LUA"; break;
            case AssetType::Font:
                bgColor = IM_COL32(60,  60,  100, 200); label = "FONT"; break;
            case AssetType::Sound:
                bgColor = IM_COL32(80,  50,  20,  200); label = "SND"; break;
            default:
                bgColor = IM_COL32(50,  50,  50,  200); label = "???"; break;
        }

        dl->AddRectFilled(pos, end, bgColor, 6.f);
        dl->AddRect(pos, end, IM_COL32(100, 100, 100, 180), 6.f);

        // Centered type label
        ImVec2 textSize = ImGui::CalcTextSize(label);
        dl->AddText(
            ImVec2(pos.x + (size - textSize.x) * 0.5f,
                   pos.y + (size - textSize.y) * 0.5f),
            IM_COL32(200, 200, 200, 255),
            label);

        // Advance cursor past the placeholder
        ImGui::Dummy(ImVec2(size, size));
    }

    //  Tooltip
    void RenderTooltip(const AssetEntry& entry)
    {
        ImGui::BeginTooltip();

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.3f, 1.f),
                            "%s", entry.filename.c_str());
        ImGui::Separator();

        ImGui::Text("Path:   %s", entry.path.c_str());
        ImGui::Text("Type:   %s", AssetTypeName(entry.type));
        ImGui::Text("Status: %s", entry.loaded ? "Loaded" : "On disk");

        if (entry.type == AssetType::Texture && entry.loaded)
        {
            ImGui::Text("Size:   %d × %d px", entry.width, entry.height);
            ImGui::Text("Handle: %u", entry.handle);
        }

        if (entry.type == AssetType::Font && entry.loaded)
        {
            ImGui::Text("Size:   %d pt", entry.fontSize);
            ImGui::Text("Handle: %u", entry.handle);
        }

        if (entry.type == AssetType::Script && entry.loaded)
        {
            ImGui::Text("Handle: %u", entry.handle);
        }

        ImGui::EndTooltip();
    }

    //  Context menu
    void RenderContextMenu(const AssetEntry& entry)
    {
        std::string menuId = "ctx_" + entry.path;
        if (!ImGui::BeginPopupContextItem(menuId.c_str())) return;

        ImGui::TextDisabled("%s", entry.filename.c_str());
        ImGui::Separator();

        if (!entry.loaded)
        {
            if (ImGui::MenuItem("Load"))
            {
                if (entry.type == AssetType::Texture)
                    m_Assets.LoadTexture(entry.path);
                else if (entry.type == AssetType::Font)
                    m_Assets.LoadFont(entry.path, 16);
                else if (entry.type == AssetType::Script)
                    m_Assets.LoadScript(entry.path);
            }
        }
        else
        {
            if (ImGui::MenuItem("Unload"))
            {
                if (entry.type == AssetType::Texture)
                    m_Assets.UnloadTexture(entry.handle);
                else if (entry.type == AssetType::Font)
                    m_Assets.UnloadFont(entry.handle);
                else if (entry.type == AssetType::Script)
                    m_Assets.UnloadScript(entry.handle);
            }
        }

        if (entry.type == AssetType::Script)
        {
            ImGui::Separator();
            if (ImGui::MenuItem("Open in Editor"))
            {
                m_Selected = entry.path;
                if (m_OnSelect) m_OnSelect(entry.path, entry.type);
            }
        }

        if (ImGui::MenuItem("Copy Path"))
            ImGui::SetClipboardText(entry.path.c_str());

        ImGui::EndPopup();
    }

    //  Utilities
    static std::string TruncateFilename(const std::string& name, int maxChars)
    {
        if (maxChars < 4) maxChars = 4;
        if ((int)name.size() <= maxChars) return name;
        return name.substr(0, maxChars - 3) + "...";
    }
};

} // namespace Cadmium::Editor.

#endif // CADMIUM_EDITOR_IMGUI_ASSET_PANEL_HPP
