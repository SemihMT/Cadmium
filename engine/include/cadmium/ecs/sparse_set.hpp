#ifndef CADMIUM_SPARSE_SET_HPP
#define CADMIUM_SPARSE_SET_HPP

#include <cadmium/ecs/entity.hpp>
#include <vector>
#include <cassert>

namespace Cadmium
{
  static constexpr uint32_t k_InvalidSparse = std::numeric_limits<uint32_t>::max();

  // Type-erased base for storing in a map
  class ISparseSet
  {
  public:
    virtual ~ISparseSet() = default;
    virtual void Remove(uint32_t index) = 0;
    virtual bool Has(uint32_t index) const = 0;
  };

  template<typename T>
  class SparseSet : public ISparseSet
  {
  public:
    void Add(uint32_t index, T component)
    {
      if (index >= m_Sparse.size())
        m_Sparse.resize(index + 1, k_InvalidSparse);

      assert(m_Sparse[index] == k_InvalidSparse && "Component already exists");

      m_Sparse[index] = static_cast<uint32_t>(m_Dense.size());
      m_Dense.push_back(index);
      m_Data.push_back(std::move(component));
    }

    void Remove(uint32_t index) override
    {
      if (!Has(index)) return;

      // Swap with last element to keep dense array packed
      uint32_t denseIndex = m_Sparse[index];
      uint32_t lastIndex  = m_Dense.back();

      m_Dense[denseIndex] = lastIndex;
      m_Data[denseIndex]  = std::move(m_Data.back());

      m_Sparse[lastIndex] = denseIndex;
      m_Sparse[index]     = k_InvalidSparse;

      m_Dense.pop_back();
      m_Data.pop_back();
    }

    bool Has(uint32_t index) const override
    {
      return index < m_Sparse.size()
          && m_Sparse[index] != k_InvalidSparse;
    }

    T& Get(uint32_t index)
    {
      assert(Has(index) && "Component not found");
      return m_Data[m_Sparse[index]];
    }

    const T& Get(uint32_t index) const
    {
      assert(Has(index) && "Component not found");
      return m_Data[m_Sparse[index]];
    }

    // Iteration - cache friendly, iterates dense array
    auto begin()       { return m_Data.begin(); }
    auto end()         { return m_Data.end(); }
    auto begin() const { return m_Data.begin(); }
    auto end()   const { return m_Data.end(); }

    size_t Size() const { return m_Dense.size(); }

    // Access parallel entity array during iteration
    const std::vector<uint32_t>& GetDense() const { return m_Dense; }

  private:
    std::vector<uint32_t> m_Sparse; // entity index → dense position
    std::vector<uint32_t> m_Dense;  // dense position → entity index
    std::vector<T>        m_Data;   // dense position → component data
  };

} // namespace Cadmium

#endif // CADMIUM_SPARSE_SET_HPP
