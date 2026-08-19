#ifndef CADMIUM_EDITOR_SCRIPT_HOT_RELOAD_HPP
#define CADMIUM_EDITOR_SCRIPT_HOT_RELOAD_HPP

#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/world.hpp>
#include <cadmium/scripting/script_host.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Cadmium::Editor
{

// Watches attached script files and hot-reloads them when the file changes.
// Tracking is by (Entity, index) since ScriptInstance lacks a source path.
class ScriptHotReloadService
{
public:
    // Call after pushing a ScriptInstance onto entity e's Script component.
    void TrackInstance(const std::string& resolvedPath, Entity e, size_t index)
    {
        m_Refs[resolvedPath].push_back({e, index});
        RecordWriteTime(resolvedPath);
    }

    // Call once per frame; internally throttled to avoid stat-ing every frame.
    void Update(World& world, float dt)
    {
        m_Accumulator += dt;
        if (m_Accumulator < k_PollIntervalSeconds)
            return;
        m_Accumulator = 0.f;

        for (auto& [path, lastWrite] : m_LastWriteTimes)
        {
            std::error_code ec;
            auto current = std::filesystem::last_write_time(path, ec);
            if (ec)
                continue; // file busy/locked; try again next poll

            if (current != lastWrite)
            {
                lastWrite = current;
                ReloadPath(world, path);
            }
        }
    }

    // Reload all tracked files regardless of write time ("Reload All Scripts").
    void ReloadAll(World& world)
    {
        for (auto& [path, refs] : m_Refs)
            ReloadPath(world, path);
    }

    // Drops all tracking. Must be called after WorldSnapshotService::Restore()
    // because entity handles are recreated and old indices become invalid.
    void Clear()
    {
        m_Refs.clear();
        m_LastWriteTimes.clear();
    }

private:
    struct InstanceRef { Entity entity; size_t index; };

    void ReloadPath(World& world, const std::string& resolvedPath)
    {
        auto it = m_Refs.find(resolvedPath);
        if (it == m_Refs.end())
            return;

        ScriptHost::LoadedScript loaded = world.GetScriptHost().LoadScript(resolvedPath);
        if (!loaded.valid)
        {
            Log::Error("ScriptHotReload", "Reload failed for '{}' - keeping the previous version running.", resolvedPath);
            return;
        }

        size_t applied = 0;
        for (auto refIt = it->second.begin(); refIt != it->second.end(); )
        {
            if (!world.IsValid(refIt->entity) || !world.HasComponent<Script>(refIt->entity))
            {
                refIt = it->second.erase(refIt);
                continue;
            }

            auto& instances = world.GetComponent<Script>(refIt->entity).instances;
            if (refIt->index >= instances.size())
            {
                refIt = it->second.erase(refIt);
                continue;
            }

            ScriptInstance& inst = instances[refIt->index];

            // Preserve old local (private field) state before overwriting.
            auto savedLocals = ScriptHost::CaptureLocals(inst);

            inst.env = loaded.env;
            // new env needs self re-injected
            inst.env["self"] = EntityHandle{&world, refIt->entity};
            inst.name = loaded.name;
            inst.onStart = loaded.onStart;
            inst.onUpdate = loaded.onUpdate;
            inst.onRender = loaded.onRender;
            inst.onDestroy = loaded.onDestroy;
            inst.fieldMetadata = loaded.fieldMetadata;
            inst.fieldOrder = loaded.fieldOrder;
            // `started` is left untouched so OnStart isn't re-triggered.

            ScriptHost::RestoreLocals(inst, savedLocals);

            ++applied;
            ++refIt;
        }

        Log::Info("ScriptHotReload", "Reloaded '{}' ({} instance(s))", resolvedPath, applied);
    }

    void RecordWriteTime(const std::string& path)
    {
        std::error_code ec;
        m_LastWriteTimes[path] = std::filesystem::last_write_time(path, ec);
    }

    static constexpr float k_PollIntervalSeconds = 0.5f;

    float m_Accumulator{0.f};
    std::unordered_map<std::string, std::vector<InstanceRef>> m_Refs;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_LastWriteTimes;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_HOT_RELOAD_HPP
