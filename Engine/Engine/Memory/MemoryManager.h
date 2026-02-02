//-----------------------------------------------------------------------------------
// file:    MemoryManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Central Memory Manager for the engine. Provides a unified interface for
//          memory allocation using pool allocators for different size classes.
//          Uses placement new for object construction and supports typed allocation.
//          All game objects and components should use this manager for memory.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "MemoryConfig.h"
#include "PoolAllocator.h"
#include <new>      // For placement new
#include <utility>  // For std::forward

namespace Engine {
    namespace Memory {

        //=================================================================================
        // MemoryManager - Central hub for all memory allocations
        //
        // The MemoryManager maintains multiple pool allocators, one for each size class.
        // When allocating, it determines the appropriate pool based on the requested size.
        // For sizes larger than the maximum pool size, it falls back to direct OS allocation.
        //
        // Key Features:
        // - Pre-allocated memory pools (no OS allocations during gameplay)
        // - Automatic pool expansion when running out of pre-allocated memory
        // - O(1) allocation and deallocation for pooled sizes
        // - Typed allocation with placement new support
        // - Thread-safe (can be made thread-safe with locks if needed)
        //
        // Usage:
        // 1. Call initialize() at game/level load time
        // 2. Use create<T>() to allocate and construct objects
        // 3. Use destroy<T>() to destruct and deallocate objects
        // 4. Call shutdown() at game/level unload time
        //=================================================================================
        class MemoryManager {
        public:
            //=================================================================================
            // Singleton Access
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Returns the singleton instance of the MemoryManager.

            Parameters:
            None.

            Returns:
            Reference to the MemoryManager instance.

            Where to call it:
            Anywhere memory allocation is needed.
            ==================================================================================*/
            static MemoryManager& instance();

            /*=================================================================================
            Function Brief:
            Checks if the singleton instance is still valid (not destroyed).
            This is used to handle static destruction order issues.

            Parameters:
            None.

            Returns:
            true if the instance is valid, false if it has been destroyed.

            Where to call it:
            Before accessing the MemoryManager during shutdown.
            ==================================================================================*/
            static bool is_instance_valid();

            //=================================================================================
            // Initialization / Shutdown
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Initializes the memory manager with pre-allocated pools.
            Should be called once at game start or level load.

            Parameters:
            initial_pool_size - Initial number of blocks per pool (default: 256).

            Returns:
            true if initialization succeeded, false otherwise.

            Where to call it:
            At game/engine initialization or level load.
            ==================================================================================*/
            bool initialize(size_t initial_pool_size = DEFAULT_BLOCKS_PER_PAGE);

            /*=================================================================================
            Function Brief:
            Shuts down the memory manager and releases all memory to the OS.
            All pointers obtained from this manager become invalid after this call.

            Parameters:
            None.

            Where to call it:
            At game/engine shutdown or level unload.
            ==================================================================================*/
            void shutdown();

            /*=================================================================================
            Function Brief:
            Checks if the memory manager has been initialized.

            Parameters:
            None.

            Returns:
            true if initialized, false otherwise.

            Where to call it:
            Before using the memory manager.
            ==================================================================================*/
            bool is_initialized() const;

            //=================================================================================
            // Raw Memory Allocation
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Allocates raw memory of the specified size.
            Uses pool allocators for small sizes, OS allocation for large sizes.

            Parameters:
            size - Number of bytes to allocate.

            Returns:
            Pointer to allocated memory, or nullptr if allocation failed.

            Where to call it:
            When raw memory is needed (prefer create<T>() for objects).
            ==================================================================================*/
            void* allocate(size_t size);

            /*=================================================================================
            Function Brief:
            Deallocates raw memory. The size must match the original allocation size.
            For pooled memory, this returns the block to the pool (not to OS).

            Parameters:
            ptr - Pointer to memory to deallocate.
            size - Size of the original allocation.

            Where to call it:
            When returning raw memory (prefer destroy<T>() for objects).
            ==================================================================================*/
            void deallocate(void* ptr, size_t size);

            //=================================================================================
            // Typed Allocation (with Placement New)
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Allocates memory for type T and constructs an object in place using
            placement new. This is the preferred way to create objects.

            Parameters:
            args - Arguments forwarded to T's constructor.

            Returns:
            Pointer to the constructed object, or nullptr if allocation failed.

            Where to call it:
            When creating game objects, components, or any heap-allocated objects.
            ==================================================================================*/
            template<typename T, typename... Args>
            T* create(Args&&... args);

            /*=================================================================================
            Function Brief:
            Destructs an object and returns its memory to the pool.
            Calls the destructor explicitly, then deallocates the memory.
            The memory is NOT returned to the OS, just to the pool.

            Parameters:
            ptr - Pointer to the object to destroy.

            Where to call it:
            When destroying game objects, components, or any heap-allocated objects.
            ==================================================================================*/
            template<typename T>
            void destroy(T* ptr);

            /*=================================================================================
            Function Brief:
            Allocates an array of type T and default-constructs all elements.

            Parameters:
            count - Number of elements in the array.

            Returns:
            Pointer to the first element, or nullptr if allocation failed.

            Where to call it:
            When creating arrays of objects.
            ==================================================================================*/
            template<typename T>
            T* create_array(size_t count);

            /*=================================================================================
            Function Brief:
            Destructs all elements in an array and returns the memory to the pool.

            Parameters:
            ptr - Pointer to the first element of the array.
            count - Number of elements in the array.

            Where to call it:
            When destroying arrays of objects.
            ==================================================================================*/
            template<typename T>
            void destroy_array(T* ptr, size_t count);

            //=================================================================================
            // Statistics
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Returns the total allocated memory across all pools (in bytes).

            Parameters:
            None.

            Returns:
            Total allocated bytes.

            Where to call it:
            For debugging or profiling.
            ==================================================================================*/
            size_t get_total_allocated() const;

            /*=================================================================================
            Function Brief:
            Returns the total used memory (allocated blocks) across all pools (in bytes).

            Parameters:
            None.

            Returns:
            Total used bytes.

            Where to call it:
            For debugging or profiling.
            ==================================================================================*/
            size_t get_total_used() const;

            /*=================================================================================
            Function Brief:
            Returns the total free memory across all pools (in bytes).

            Parameters:
            None.

            Returns:
            Total free bytes.

            Where to call it:
            For debugging or profiling.
            ==================================================================================*/
            size_t get_total_free() const;

            /*=================================================================================
            Function Brief:
            Prints memory statistics to the logger.

            Parameters:
            None.

            Where to call it:
            For debugging.
            ==================================================================================*/
            void print_statistics() const;

            //=================================================================================
            // Level Management
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Resets all pools - deallocates all blocks but keeps the pages.
            Useful for level transitions where all entities are destroyed.

            Parameters:
            None.

            Where to call it:
            When transitioning between levels.
            ==================================================================================*/
            void reset_pools();

        private:
            // Private constructor for singleton
            MemoryManager();
            ~MemoryManager();

            // Disable copy
            MemoryManager(const MemoryManager&) = delete;
            MemoryManager& operator=(const MemoryManager&) = delete;

            // Pool allocators for each size class (no STL!)
            // Index 0 = 16 bytes, Index 1 = 32 bytes, Index 2 = 64 bytes, etc.
            PoolAllocator* m_pools[NUM_SIZE_CLASSES];

            // Track large allocations that don't fit in pools
            // We don't use a container - just count for statistics
            size_t m_large_allocation_count;
            size_t m_large_allocation_bytes;

            bool m_initialized;
        };

        //=================================================================================
        // Template Implementation
        //=================================================================================

        template<typename T, typename... Args>
        T* MemoryManager::create(Args&&... args) {
            // Allocate raw memory
            void* memory = allocate(sizeof(T));
            if (!memory) {
                return nullptr;
            }

            // Construct the object in place using placement new
            return new (memory) T(std::forward<Args>(args)...);
        }

        template<typename T>
        void MemoryManager::destroy(T* ptr) {
            if (!ptr) return;

            // Call the destructor explicitly
            ptr->~T();

            // Return memory to pool (NOT to OS!)
            deallocate(ptr, sizeof(T));
        }

        template<typename T>
        T* MemoryManager::create_array(size_t count) {
            if (count == 0) return nullptr;

            // Allocate memory for all elements
            size_t total_size = sizeof(T) * count;
            void* memory = allocate(total_size);
            if (!memory) {
                return nullptr;
            }

            // Construct all elements using placement new
            T* array = static_cast<T*>(memory);
            for (size_t i = 0; i < count; ++i) {
                new (&array[i]) T();
            }

            return array;
        }

        template<typename T>
        void MemoryManager::destroy_array(T* ptr, size_t count) {
            if (!ptr || count == 0) return;

            // Call destructors in reverse order
            for (size_t i = count; i-- > 0; ) {
                ptr[i].~T();
            }

            // Return memory to pool
            deallocate(ptr, sizeof(T) * count);
        }

        //=================================================================================
        // Convenience Macros
        //=================================================================================

        // Macros for easier usage (optional - can use MemoryManager::instance() directly)
#define MM_CREATE(Type, ...) Engine::Memory::MemoryManager::instance().create<Type>(__VA_ARGS__)
#define MM_DESTROY(ptr) Engine::Memory::MemoryManager::instance().destroy(ptr)
#define MM_CREATE_ARRAY(Type, count) Engine::Memory::MemoryManager::instance().create_array<Type>(count)
#define MM_DESTROY_ARRAY(ptr, count) Engine::Memory::MemoryManager::instance().destroy_array(ptr, count)
#define MM_ALLOC(size) Engine::Memory::MemoryManager::instance().allocate(size)
#define MM_FREE(ptr, size) Engine::Memory::MemoryManager::instance().deallocate(ptr, size)

    } // namespace Memory
} // namespace Engine
