#ifndef CADMIUM_REGISTRY_HPP
#define CADMIUM_REGISTRY_HPP

#include <cadmium/ecs/component_id.hpp>
#include <cadmium/ecs/entity.hpp>
#include <cadmium/ecs/sparse_set.hpp>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>


namespace Cadmium
{
class Registry
{
  public:
    // Entity lifecycle
    Entity CreateEntity()
    {
        uint32_t index{};
        uint32_t generation{};

        if (!m_FreeList.empty())
        {
            index = m_FreeList.front();
            m_FreeList.pop();
            generation = m_Generations[index];
        }
        else
        {
            index = static_cast<uint32_t>(m_Generations.size());
            if (index > EntityBits::k_MaxIndex)
                throw std::runtime_error("Entity limit reached");
            m_Generations.push_back(0);
            generation = 0;
        }

        return MakeEntity(index, generation);
    }

    void DestroyEntity(Entity entity)
    {
        if (!IsValid(entity))
            return;

        uint32_t index = EntityIndex(entity);

        // Remove all components for this entity
        for (auto& [id, set] : m_Sets)
            set->Remove(index);

        // Increment generation to invalidate existing IDs
        uint32_t& gen = m_Generations[index];
        if (gen < EntityBits::k_MaxGeneration)
            gen++;

        m_FreeList.push(index);
    }

    bool IsValid(Entity entity) const
    {
        if (entity == k_NullEntity)
            return false;

        uint32_t index = EntityIndex(entity);

        if (index >= m_Generations.size())
            return false;

        return EntityGeneration(entity) == m_Generations[index];
    }

    // Component management
    template <typename T> void AddComponent(Entity entity, T component)
    {
        if (!IsValid(entity))
            throw std::runtime_error("AddComponent called on invalid entity");

        auto& set = GetOrCreateSet<T>();
        set.Add(EntityIndex(entity), std::move(component));
    }

    template <typename T> void RemoveComponent(Entity entity)
    {
        if (!IsValid(entity))
            return;

        auto it = m_Sets.find(GetComponentID<T>());
        if (it != m_Sets.end())
            it->second->Remove(EntityIndex(entity));
    }

    template <typename T> bool HasComponent(Entity entity) const
    {
        if (!IsValid(entity))
            return false;
        auto it = m_Sets.find(GetComponentID<T>());
        return it != m_Sets.end() && it->second->Has(EntityIndex(entity));
    }

    template <typename T> T& GetComponent(Entity entity)
    {
        if (!IsValid(entity))
            throw std::runtime_error("GetComponent called on invalid entity");

        auto it = m_Sets.find(GetComponentID<T>());
        if (it == m_Sets.end())
            throw std::runtime_error("Component type not found");

        return static_cast<SparseSet<T>*>(it->second.get())->Get(EntityIndex(entity));
    }

    template <typename T> const T& GetComponent(Entity entity) const
    {
        if (!IsValid(entity))
            throw std::runtime_error("GetComponent called on invalid entity");

        auto it = m_Sets.find(GetComponentID<T>());
        if (it == m_Sets.end())
            throw std::runtime_error("Component type not found");

        return static_cast<const SparseSet<T>*>(it->second.get())->Get(EntityIndex(entity));
    }

    template <typename T> T* TryGetComponent(Entity entity)
    {
        if (!HasComponent<T>(entity))
            return nullptr;
        return &GetComponent<T>(entity);
    }

    // Query - returns pairs of (Entity, T&) for all entities with T
    template <typename T> std::vector<std::pair<Entity, T*>> Query()
    {
        std::vector<std::pair<Entity, T*>> result;

        auto it = m_Sets.find(GetComponentID<T>());
        if (it == m_Sets.end())
            return result;

        auto* set = static_cast<SparseSet<T>*>(it->second.get());
        for (uint32_t index : set->GetDense())
            result.emplace_back(MakeEntity(index, m_Generations[index]), &set->Get(index));
        return result;
    }

    // Multi-component query - entities with ALL of T...
    template <typename T, typename... Rest> std::vector<Entity> QueryEntities()
    {
        std::vector<Entity> result;

        auto it = m_Sets.find(GetComponentID<T>());
        if (it == m_Sets.end())
            return result;

        auto* set = static_cast<SparseSet<T>*>(it->second.get());
        for (uint32_t index : set->GetDense())
        {
            Entity entity = MakeEntity(index, m_Generations[index]);
            if ((HasComponent<Rest>(entity) && ...))
                result.push_back(entity);
        }

        return result;
    }

    size_t EntityCount() const { return m_Generations.size() - m_FreeList.size(); }

  private:
    template <typename T> SparseSet<T>& GetOrCreateSet()
    {
        ComponentID id = GetComponentID<T>();
        auto it = m_Sets.find(id);

        if (it == m_Sets.end())
        {
            auto [inserted, _] = m_Sets.emplace(id, std::make_unique<SparseSet<T>>());
            return *static_cast<SparseSet<T>*>(inserted->second.get());
        }

        return *static_cast<SparseSet<T>*>(it->second.get());
    }

    std::unordered_map<ComponentID, std::unique_ptr<ISparseSet>> m_Sets;
    std::vector<uint32_t> m_Generations;
    std::queue<uint32_t> m_FreeList;
};

} // namespace Cadmium

#endif // CADMIUM_REGISTRY_HPP
