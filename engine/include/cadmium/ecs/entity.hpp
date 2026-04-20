#ifndef CADMIUM_ENTITY_HPP
#define CADMIUM_ENTITY_HPP

#include <cstdint>
#include <limits>

namespace Cadmium
{
  // Entity ID layout:
  // lower 20 bits = index  (max 1,048,576 entities)
  // upper 12 bits = generation (max 4,096 reuses per slot)

  using Entity = uint32_t;

  namespace EntityBits
  {
    static constexpr uint32_t k_IndexBits      = 20;
    static constexpr uint32_t k_GenerationBits = 12;

    static constexpr uint32_t k_IndexMask      = (1u << k_IndexBits) - 1;
    static constexpr uint32_t k_GenerationMask = (1u << k_GenerationBits) - 1;

    static constexpr uint32_t k_MaxIndex      = k_IndexMask;
    static constexpr uint32_t k_MaxGeneration = k_GenerationMask;
  }

  static constexpr Entity k_NullEntity = std::numeric_limits<Entity>::max();

  inline uint32_t EntityIndex(Entity e)
  {
    return e & EntityBits::k_IndexMask;
  }

  inline uint32_t EntityGeneration(Entity e)
  {
    return (e >> EntityBits::k_IndexBits) & EntityBits::k_GenerationMask;
  }

  inline Entity MakeEntity(uint32_t index, uint32_t generation)
  {
    return (generation << EntityBits::k_IndexBits) | (index & EntityBits::k_IndexMask);
  }

} // namespace Cadmium

#endif // CADMIUM_ENTITY_HPP
