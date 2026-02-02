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

        MemoryManager& MemoryManager::instance() {
            static MemoryManager instance;
            return instance;
        }

        //=================================================================================
        // Constructor / Destructor
        //=================================================================================

        MemoryManager::MemoryManager()
            : m_large_allocation_count(0)
            , m_large_allocation_bytes(0)
            , m_initialized(false)
        {
            // Initialize all pool pointers to nullptr
            for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
                m_pools[i] = nullptr;
            }
        }

        MemoryManager::~MemoryManager() {
            shutdown();
        }

        //=================================================================================
        // Initialization / Shutdown
        //=================================================================================

        bool MemoryManager::initialize(size_t initial_pool_size) {
            if (m_initialized) {
                Log::memory("MemoryManager already initialized - skipping");
                return true;
            }

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

            // Check if we can use a pool allocator
            if (is_poolable_size(size)) {
                size_t pool_index = get_size_class_index(size);

                if (m_pools[pool_index] && m_pools[pool_index]->is_initialized()) {
                    return m_pools[pool_index]->allocate();
                }
            }

            // Fall back to OS allocation for large sizes
            // This should be rare - log it for tracking
            Log::memory("Large allocation (OS): {} bytes - bypassing pool system", size);
            void* ptr = ::operator new(size, std::nothrow);
            if (ptr) {
                ++m_large_allocation_count;
                m_large_allocation_bytes += size;
            }
            return ptr;
        }

        void MemoryManager::deallocate(void* ptr, size_t size) {
            if (!ptr || size == 0) {
                return;
            }

            // Check if this came from a pool allocator
            if (is_poolable_size(size)) {
                size_t pool_index = get_size_class_index(size);

                if (m_pools[pool_index] && m_pools[pool_index]->is_initialized()) {
                    m_pools[pool_index]->deallocate(ptr);
                    return;
                }
            }

            // Fall back to OS deallocation for large sizes
            Log::memory("Large deallocation (OS): {} bytes returned to system", size);
            ::operator delete(ptr);
            if (m_large_allocation_count > 0) {
                --m_large_allocation_count;
                m_large_allocation_bytes -= size;
            }
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
