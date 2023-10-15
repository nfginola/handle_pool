#ifndef _HANDLE_STORAGE_POOL
#define _HANDLE_STORAGE_POOL

#include "handle_pool.h"
#include <functional>

// Helper class for pairing a handle pool with a storage pool
template <typename HANDLE_TYPE, typename STORAGE_TYPE> class HandleStoragePool
{
public:
   HandleStoragePool()
   {
      // 0:th always reserved
      m_storage.push_back({});
   }

   std::optional<Handle<HANDLE_TYPE>> allocate()
   {
      Handle<HANDLE_TYPE> handle;

      auto opt = m_handles.allocate();
      if (!opt)
         return std::nullopt;
      handle = opt.value();

      return handle;
   }

   void insert(Handle<HANDLE_TYPE> handle, const STORAGE_TYPE &data)
   {
      // Match storage size
      if (m_storage.capacity() <= handle.index())
         m_storage.resize(2 * m_storage.capacity());

      m_storage[handle.index()] = data;
   }

   void free(Handle<HANDLE_TYPE> handle, std::optional<std::function<void(STORAGE_TYPE &)>> func = std::nullopt)
   {
      if (func)
         func.value()(m_storage[handle.index()]);
      m_handles.free(handle);
   }

   STORAGE_TYPE &get(Handle<HANDLE_TYPE> handle)
   {
      return m_storage[handle.index()];
   }

   void iterate_storage(const std::function<void(std::vector<STORAGE_TYPE> &)> &func)
   {
      func(m_storage);
   }

private:
   HandlePool<HANDLE_TYPE> m_handles;
   std::vector<STORAGE_TYPE> m_storage;
};

#endif
