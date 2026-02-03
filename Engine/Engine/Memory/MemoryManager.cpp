//-----------------------------------------------------------------------------------
// file:    MemoryManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the central Memory Manager. Manages pool allocators
//          for different size classes and handles large allocations through the OS.
//          All memory for game objects and components should flow through here.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "MemoryManager.h"
#include "../Logger.h"
#include <new>

namespace Engine {
  namespace Memory {

    //=================================================================================
    // Singleton Implementation
    //=================================================================================

    // Static flag to track if the MemoryManager singleton has been destroyed
    // This helps handle the static destruction order fiasco
    static bool s_instance_destroyed = false;

    bool MemoryManager::is_instance_valid() {
      return !s_instance_destroyed;
    }

    MemoryManager& MemoryManager::instance() {
      static MemoryManager instance;
      return instance;
    }

    //=================================================================================
    // Constructor / Destructor
    //=================================================================================

    MemoryManager::MemoryManager()
      : m_os_allocation_count(0)
      , m_large_allocation_count(0)
      , m_large_allocation_bytes(0)
      , m_initialized(false)
    {
      // Initialize all pool pointers to nullptr
      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        m_pools[i] = nullptr;
      }
      // Initialize OS allocation tracking
      for (size_t i = 0; i < MAX_OS_ALLOCATIONS; ++i) {
        m_os_allocations[i].ptr = nullptr;
        m_os_allocations[i].size = 0;
      }
    }

    MemoryManager::~MemoryManager() {
      shutdown();
      // Mark the instance as destroyed to prevent access after destruction
      s_instance_destroyed = true;
    }

    // Helper to find an OS allocation by pointer
    size_t MemoryManager::find_os_allocation(void* ptr) const {
      for (size_t i = 0; i < m_os_allocation_count; ++i) {
        if (m_os_allocations[i].ptr == ptr) {
          return i;
        }
      }
      return MAX_OS_ALLOCATIONS; // Not found sentinel
    }

    //=================================================================================
    // Initialization / Shutdown
    //=================================================================================

    bool MemoryManager::initialize(size_t initial_pool_size) {
      if (m_initialized) {
        Log::memory("MemoryManager already initialized - skipping");
        return true;
      }

      // Reset the destroyed flag in case of re-initialization
      s_instance_destroyed = false;

      Log::memory("=== Memory Manager Initialization ===");
      Log::memory("Configuration:");
      Log::memory("  Size classes: {}", NUM_SIZE_CLASSES);
      Log::memory("  Initial blocks per pool: {}", initial_pool_size);
      Log::memory("  Min block size: {} bytes", MIN_BLOCK_SIZE);
      Log::memory("  Max poolable size: {} bytes", get_block_size_for_class(NUM_SIZE_CLASSES - 1));

      // Calculate total pre-allocated memory
      size_t total_preallocated = 0;

      // Create pool allocators for each size class
      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        size_t block_size = get_block_size_for_class(i);

        // Allocate memory for the PoolAllocator struct
        void* pool_memory = ::operator new(sizeof(PoolAllocator), std::nothrow);
        if (!pool_memory) {
          Log::memory("ERROR: Failed to allocate pool struct for size class {}", i);
          shutdown();
          return false;
        }

        // Construct the pool in place
        m_pools[i] = new (pool_memory) PoolAllocator();

        // Initialize the pool
        if (!m_pools[i]->initialize(block_size, initial_pool_size)) {
          Log::memory("ERROR: Failed to initialize pool {} (block size: {} bytes)", i, block_size);
          shutdown();
          return false;
        }

        size_t pool_bytes = block_size * initial_pool_size;
        total_preallocated += pool_bytes;
        Log::memory("  Pool[{}]: {} bytes x {} blocks = {} KB",
          i, block_size, initial_pool_size, pool_bytes / 1024);
      }

      m_initialized = true;
      Log::memory("Total pre-allocated memory: {} KB ({} MB)",
        total_preallocated / 1024, total_preallocated / (1024 * 1024));
      Log::memory("=== Memory Manager Ready ===");

      return true;
    }

    void MemoryManager::shutdown() {
      if (!m_initialized && m_pools[0] == nullptr) {
        return;
      }

      Log::memory("=== Memory Manager Shutdown ===");

      // Collect final statistics before shutdown
      size_t total_allocated = get_total_allocated();
      size_t total_used = get_total_used();

      // Destroy all pool allocators
      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        if (m_pools[i]) {
          size_t pool_used = m_pools[i]->get_allocated_blocks();
          size_t pool_total = m_pools[i]->get_total_blocks();
          if (pool_used > 0) {
            Log::memory("  Pool[{}]: {} blocks still allocated (potential leak)", i, pool_used);
          }
          m_pools[i]->shutdown();
          m_pools[i]->~PoolAllocator();
          ::operator delete(m_pools[i]);
          m_pools[i] = nullptr;
        }
      }

      // Log large allocation statistics
      if (m_large_allocation_count > 0) {
        Log::memory("  Large (OS) allocations remaining: {} ({} bytes)",
          m_large_allocation_count, m_large_allocation_bytes);
      }

      Log::memory("Final stats - Allocated: {} KB, Used: {} KB",
        total_allocated / 1024, total_used / 1024);

      m_large_allocation_count = 0;
      m_large_allocation_bytes = 0;
      m_initialized = false;

      // NOTE: We do NOT set s_instance_destroyed = true here!
      // Containers may still need to deallocate during static destruction.
      // The deallocate() function checks if pools are null and falls back to OS delete.
      // s_instance_destroyed is only set in the destructor.

      Log::memory("=== Memory Manager Shut Down ===");
    }

    bool MemoryManager::is_initialized() const {
      return m_initialized;
    }

    //=================================================================================
    // Raw Memory Allocation
    //=================================================================================

    void* MemoryManager::allocate(size_t size) {
      if (size == 0) {
        return nullptr;
      }

      // Check if MemoryManager is being destroyed (static destruction order issue)
      if (s_instance_destroyed) {
        // Fall back to OS allocation - this should be very rare
        return ::operator new(size, std::nothrow);
      }

      // Check if we can use a pool allocator
      if (is_poolable_size(size)) {
        size_t pool_index = get_size_class_index(size);

        if (m_pools[pool_index] && m_pools[pool_index]->is_initialized()) {
          return m_pools[pool_index]->allocate();
        }
      }

      // Fall back to OS allocation for large sizes OR when pools aren't ready
      // This should be rare - log it for tracking
      Log::memory("Large allocation (OS): {} bytes - bypassing pool system", size);
      void* ptr = ::operator new(size, std::nothrow);
      if (ptr) {
        // Track this OS allocation so we can properly deallocate it later
        if (m_os_allocation_count < MAX_OS_ALLOCATIONS) {
          m_os_allocations[m_os_allocation_count].ptr = ptr;
          m_os_allocations[m_os_allocation_count].size = size;
          ++m_os_allocation_count;
        }
        ++m_large_allocation_count;
        m_large_allocation_bytes += size;
      }
      return ptr;
    }

    void MemoryManager::deallocate(void* ptr, size_t size) {
      if (!ptr || size == 0) {
        return;
      }

      // Check if MemoryManager singleton is being destroyed (static destruction order issue)
      if (s_instance_destroyed) {
        // Memory Manager destructor is running - just use OS delete
        ::operator delete(ptr);
        return;
      }

      // Check if this came from a pool allocator and pool is still valid
      if (is_poolable_size(size)) {
        size_t pool_index = get_size_class_index(size);

        // Check if pool exists and is initialized (may be null after shutdown())
        if (m_pools[pool_index] && m_pools[pool_index]->is_initialized()) {
          m_pools[pool_index]->deallocate(ptr);
          return;
        }
        // Pool was freed during shutdown() - fall through to OS delete
      }

      // Check if this is a tracked OS allocation
      size_t os_index = find_os_allocation(ptr);
      if (os_index < MAX_OS_ALLOCATIONS) {
        // Found in OS allocation tracker - deallocate via OS
        Log::memory("Large deallocation (OS): {} bytes returned to system", m_os_allocations[os_index].size);
        ::operator delete(ptr);

        // Update tracking
        if (m_large_allocation_count > 0) {
          --m_large_allocation_count;
          m_large_allocation_bytes -= m_os_allocations[os_index].size;
        }

        // Remove from tracker by swapping with last element
        m_os_allocations[os_index] = m_os_allocations[m_os_allocation_count - 1];
        m_os_allocations[m_os_allocation_count - 1].ptr = nullptr;
        m_os_allocations[m_os_allocation_count - 1].size = 0;
        --m_os_allocation_count;
        return;
      }

      // Not tracked and not in a valid pool - must be:
      // 1. A large allocation made after tracker was full, OR
      // 2. A pooled allocation being freed after shutdown() freed the pools
      // Either way, use OS delete
      ::operator delete(ptr);
    }

    //=================================================================================
    // Statistics
    //=================================================================================

    size_t MemoryManager::get_total_allocated() const {
      size_t total = 0;

      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        if (m_pools[i] && m_pools[i]->is_initialized()) {
          total += m_pools[i]->get_total_blocks() * m_pools[i]->get_block_size();
        }
      }

      total += m_large_allocation_bytes;
      return total;
    }

    size_t MemoryManager::get_total_used() const {
      size_t used = 0;

      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        if (m_pools[i] && m_pools[i]->is_initialized()) {
          used += m_pools[i]->get_allocated_blocks() * m_pools[i]->get_block_size();
        }
      }

      used += m_large_allocation_bytes;
      return used;
    }

    size_t MemoryManager::get_total_free() const {
      size_t free = 0;

      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        if (m_pools[i] && m_pools[i]->is_initialized()) {
          free += m_pools[i]->get_free_blocks() * m_pools[i]->get_block_size();
        }
      }

      return free;
    }

    void MemoryManager::print_statistics() const {
      Log::memory("=== Memory Manager Statistics ===");
      Log::memory("Total allocated: {} KB", get_total_allocated() / 1024);
      Log::memory("Total used: {} KB", get_total_used() / 1024);
      Log::memory("Total free: {} KB", get_total_free() / 1024);

      Log::memory("--- Pool Details ---");
      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        if (m_pools[i] && m_pools[i]->is_initialized()) {
          size_t block_size = m_pools[i]->get_block_size();
          size_t total = m_pools[i]->get_total_blocks();
          size_t used = m_pools[i]->get_allocated_blocks();
          size_t pages = m_pools[i]->get_page_count();
          float utilization = total > 0 ? (static_cast<float>(used) / total * 100.0f) : 0.0f;

          Log::memory("  Pool[{}] ({} B): {}/{} blocks ({:.1f}%), {} pages",
            i, block_size, used, total, utilization, pages);
        }
      }

      if (m_large_allocation_count > 0) {
        Log::memory("--- Large Allocations (OS) ---");
        Log::memory("  Count: {}", m_large_allocation_count);
        Log::memory("  Bytes: {} ({} KB)", m_large_allocation_bytes, m_large_allocation_bytes / 1024);
      }

      Log::memory("=================================");
    }

    //=================================================================================
    // Level Management
    //=================================================================================

    void MemoryManager::reset_pools() {
      Log::memory("=== Resetting Memory Manager Pools ===");

      for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        if (m_pools[i] && m_pools[i]->is_initialized()) {
          size_t freed = m_pools[i]->get_allocated_blocks();
          m_pools[i]->reset();
          if (freed > 0) {
            Log::memory("  Pool[{}]: Reset {} blocks", i, freed);
          }
        }
      }

      // Note: Large allocations are not tracked in a way that allows reset
      // They must be deallocated individually
      if (m_large_allocation_count > 0) {
        Log::memory("  Warning: {} large allocations ({} bytes) not reset - must be freed manually",
          m_large_allocation_count, m_large_allocation_bytes);
      }

      Log::memory("=== Memory Manager Pools Reset ===");
    }

  } // namespace Memory
} // namespace Engine
