#ifndef _HANDLE_POOL
#define _HANDLE_POOL
#include <assert.h>
#include <cstdint>
#include <limits>
#include <optional>
#include <queue>

// Debug
#include <stdio.h>

namespace
{
using FullKey = std::uint64_t;
using HalfKey = std::uint32_t;

#define HANDLE_DEF_INDEX_MASK() \
   static constexpr FullKey INDEX_MASK = ((FullKey)1 << std::numeric_limits<::HalfKey>::digits) - 1;

#define HANDLE_DEF_GEN_MASK() static constexpr FullKey GEN_MASK = ~INDEX_MASK;

#define HANDLE_DEF_HALF_KEY_SHIFT() \
   static constexpr std::uint8_t HALF_KEY_SHIFT = std::numeric_limits<::HalfKey>::digits;

#define HANDLE_DEFS()      \
   HANDLE_DEF_INDEX_MASK() \
   HANDLE_DEF_GEN_MASK()   \
   HANDLE_DEF_HALF_KEY_SHIFT()

static_assert(std::numeric_limits<::HalfKey>::digits == std::numeric_limits<::FullKey>::digits / 2,
              "Index type must be half of Handle type");

} // namespace

template <typename T> struct Handle
{
   ::FullKey key;
   HANDLE_DEFS()

   ::HalfKey index() const
   {
      return key & INDEX_MASK;
   }

   ::HalfKey gen() const
   {
      return (key & GEN_MASK) >> HALF_KEY_SHIFT;
   }

   operator uint64_t() const
   {
      return key;
   }
};

template <typename T> class HandlePool
{
   HANDLE_DEFS()
public:
   void print_internals()
   {
      printf("Next index: %u\n", m_next_index);
      printf("Queue size: %lu\n", m_reusables.size());
      printf("Gen size: %lu\n", m_gen_counters.capacity());
      printf("Slots are full?: %s\n", m_slots_full ? "yes" : "no");
      printf("====\n");
   };

   std::optional<Handle<T>> allocate()
   {
      // [ Generational Counter --- Index ]
      // Lower half = Index
      // Upper half = Generational counter

      Handle<T> handle;
      if (m_reusables.empty())
      {
         // Generation always 0
         ::HalfKey index = m_next_index;

         if (m_slots_full)
            return std::nullopt;

         // Tag overflow
         m_slots_full = m_next_index == std::numeric_limits<::HalfKey>::max();

         if (!m_slots_full)
            ++m_next_index;

         // Reserve generation tracking
         m_gen_counters.push_back(0);

         handle.key = index;
      }
      else
      {
         ::FullKey recycled = m_reusables.front();
         m_reusables.pop();
         handle.key = recycled;
      }

      return handle;
   }

   void free(Handle<T> handle)
   {
      const ::HalfKey index = handle.index();

      assert(index < m_next_index);
      assert(m_gen_counters[index] == handle.gen() &&
             "Double-free detected"); // Ensure that the handle is not double-freed

      // Try to increase generation
      ::HalfKey gen = (handle.key & GEN_MASK) >> HALF_KEY_SHIFT;
      const bool at_max = gen == std::numeric_limits<::HalfKey>::max();
      if (!at_max)
      {
         // Increase generation and recycle
         ++gen;
         ::FullKey new_key = (FullKey)index | ((FullKey)gen << HALF_KEY_SHIFT);
         m_reusables.push(new_key);

         // Update generation tracking
         m_gen_counters[index] = gen;
      }
   }

   // For user to check for use-after-free
   bool is_valid(Handle<T> handle) const
   {
      return m_gen_counters[handle.index()] == handle.gen();
   }

private:
   ::HalfKey m_next_index = 0;
   std::queue<::FullKey> m_reusables;
   std::vector<std::uint32_t> m_gen_counters;
   bool m_slots_full = false;
};

#endif
