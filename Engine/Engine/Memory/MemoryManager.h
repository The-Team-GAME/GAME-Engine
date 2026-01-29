//-----------------------------------------------------------------------------------
// file:    MemoryManager.h
// project: Monkey Stole My GPA
//
// author:  [Your Name] (100%)
//          Primary author
//
// email:   [your.email]@digipen.edu
//
// brief:   Custom Memory Manager implementing a Pool Allocator with Free List.
//          This memory manager pre-allocates memory pages at load time and uses
//          placement new for object construction. No runtime OS allocations unless
//          pre-allocated memory runs out, at which point new pages are added.
//
//          Key Features:
//          - Pool Allocator: Fixed-size block allocation for components/entities
//          - Free Linked List: O(1) allocation and deallocation
//          - Memory Pages: Large contiguous memory blocks
//          - Placement New: Objects constructed in pre-allocated memory
//          - Dynamic Extension: Automatically adds pages when needed
//          - NO STL CONTAINERS in the allocator implementation itself
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../TypeDef.h"
#include <new>      // For placement new
#include <cstdlib>  // For malloc/free (initial page allocation only)
#include <cstring>  // For memset
#include <utility>  // For std::forward

namespace Engine {
  namespace Memory {

    // ============================================================================
    // Constants
    // ============================================================================

    /// Default page size (4MB) - can be adjusted based on game requirements
    inline constexpr size_t DEFAULT_PAGE_SIZE = 4 * 1024 * 1024;

    /// Minimum block size to ensure free list pointer can be stored
    inline constexpr size_t MIN_BLOCK_SIZE = sizeof(void*);

    /// Alignment for memory blocks (typically cache line aligned)
    inline constexpr size_t DEFAULT_ALIGNMENT = 16;

    // ============================================================================
    // FreeListNode - Embedded in free blocks (no extra memory overhead)
    // ============================================================================

    /**
     * @brief Node structure embedded within free memory blocks.
     *
     * When a block is free, we store this structure at its start.
     * When allocated, this memory is overwritten by the actual object.
     * This technique avoids any additional memory overhead for tracking free blocks.
     */
    struct FreeListNode {
      FreeListNode* next;  ///< Pointer to the next free block
    };

    // ============================================================================
    // MemoryPage - A contiguous chunk of pre-allocated memory
    // ============================================================================

    /**
     * @brief Represents a single page of pre-allocated memory.
     *
     * Memory pages are linked together to form a chain. Each page contains
     * raw memory that is divided into fixed-size blocks by the Pool Allocator.
     *
     * Layout:
     * [MemoryPage header][Block 0][Block 1][Block 2]...[Block N]
     */
    struct MemoryPage {
      MemoryPage* next_page;    ///< Pointer to the next page (linked list of pages)
      u8* memory;               ///< Pointer to the raw memory in this page
      size_t size;              ///< Total size of usable memory in bytes
      size_t used;              ///< Amount of memory currently in use (for debugging)

      /**
       * @brief Creates a new memory page with the specified size.
       * @param page_size Size of the page in bytes.
       * @return Pointer to the newly created page, or nullptr on failure.
       */
      static MemoryPage* create(size_t page_size) {
        // Allocate memory for the page header + actual memory
        void* raw = std::malloc(sizeof(MemoryPage) + page_size);
        if (!raw) return nullptr;

        MemoryPage* page = static_cast<MemoryPage*>(raw);
        page->next_page = nullptr;
        page->memory = reinterpret_cast<u8*>(raw) + sizeof(MemoryPage);
        page->size = page_size;
        page->used = 0;

        // Zero initialize for safety
        std::memset(page->memory, 0, page_size);

        return page;
      }

      /**
       * @brief Destroys a memory page and frees its memory.
       * @param page The page to destroy.
       */
      static void destroy(MemoryPage* page) {
        if (page) {
          std::free(page);
        }
      }
    };

    // ============================================================================
    // PoolAllocator - Fixed-size block allocator with free list
    // ============================================================================

    /**
     * @brief Pool Allocator for fixed-size memory blocks.
     *
     * This allocator manages memory for objects of a specific size. It uses:
     * - Memory Pages: Pre-allocated chunks of memory
     * - Free List: Linked list of available blocks (embedded in the blocks themselves)
     * - Placement New: Objects are constructed in pre-allocated memory
     *
     * Time Complexity:
     * - Allocate: O(1) - just pop from free list
     * - Deallocate: O(1) - just push to free list
     *
     * When the free list is empty and all pages are full, a new page is allocated.
     *
     * @note This class does NOT use any STL containers internally.
     */
    class PoolAllocator {
    private:
      MemoryPage* first_page;     ///< Head of the page linked list
      MemoryPage* current_page;   ///< Current page being allocated from
      FreeListNode* free_list;    ///< Head of the free list

      size_t block_size;          ///< Size of each block (aligned)
      size_t blocks_per_page;     ///< Number of blocks that fit in one page
      size_t page_size;           ///< Size of each page

      // Statistics for debugging
      size_t total_allocations;   ///< Total number of allocations made
      size_t active_allocations;  ///< Currently active (non-freed) allocations
      size_t page_count;          ///< Number of pages allocated

      /**
       * @brief Aligns a size to the specified alignment boundary.
       * @param size The size to align.
       * @param alignment The alignment boundary.
       * @return The aligned size.
       */
      static size_t align_size(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
      }

      /**
       * @brief Adds a new memory page to the allocator.
       *
       * This is called when we run out of pre-allocated memory.
       * The new page is divided into blocks and added to the free list.
       *
       * @return true if page was successfully added, false otherwise.
       */
      bool add_page() {
        MemoryPage* new_page = MemoryPage::create(page_size);
        if (!new_page) {
          return false;
        }

        // Link the new page
        new_page->next_page = first_page;
        first_page = new_page;
        current_page = new_page;
        ++page_count;

        // Divide the page into blocks and add to free list
        initialize_page_blocks(new_page);

        return true;
      }

      /**
       * @brief Initializes all blocks in a page and adds them to the free list.
       * @param page The page to initialize.
       */
      void initialize_page_blocks(MemoryPage* page) {
        u8* block_ptr = page->memory;

        // Create blocks and link them into the free list
        for (size_t i = 0; i < blocks_per_page; ++i) {
          FreeListNode* node = reinterpret_cast<FreeListNode*>(block_ptr);
          node->next = free_list;
          free_list = node;
          block_ptr += block_size;
        }
      }

    public:
      /**
       * @brief Constructs a pool allocator for blocks of the specified size.
       * @param element_size Size of each element to allocate.
       * @param initial_page_size Size of initial memory page (default: 4MB).
       * @param alignment Memory alignment for blocks (default: 16 bytes).
       */
      PoolAllocator(size_t element_size,
        size_t initial_page_size = DEFAULT_PAGE_SIZE,
        size_t alignment = DEFAULT_ALIGNMENT)
        : first_page(nullptr)
        , current_page(nullptr)
        , free_list(nullptr)
        , total_allocations(0)
        , active_allocations(0)
        , page_count(0)
      {
        // Ensure block size can hold a free list pointer and is aligned
        block_size = align_size(
          (element_size < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : element_size,
          alignment
        );

        page_size = initial_page_size;
        blocks_per_page = page_size / block_size;

        // Pre-allocate the initial page
        add_page();
      }

      /**
       * @brief Destructor - frees all allocated pages.
       */
      ~PoolAllocator() {
        // Free all pages
        MemoryPage* page = first_page;
        while (page) {
          MemoryPage* next = page->next_page;
          MemoryPage::destroy(page);
          page = next;
        }
      }

      // Disable copy operations
      PoolAllocator(const PoolAllocator&) = delete;
      PoolAllocator& operator=(const PoolAllocator&) = delete;

      // Enable move operations
      PoolAllocator(PoolAllocator&& other) noexcept
        : first_page(other.first_page)
        , current_page(other.current_page)
        , free_list(other.free_list)
        , block_size(other.block_size)
        , blocks_per_page(other.blocks_per_page)
        , page_size(other.page_size)
        , total_allocations(other.total_allocations)
        , active_allocations(other.active_allocations)
        , page_count(other.page_count)
      {
        other.first_page = nullptr;
        other.current_page = nullptr;
        other.free_list = nullptr;
      }

      PoolAllocator& operator=(PoolAllocator&& other) noexcept {
        if (this != &other) {
          // Free existing pages
          MemoryPage* page = first_page;
          while (page) {
            MemoryPage* next = page->next_page;
            MemoryPage::destroy(page);
            page = next;
          }

          // Move from other
          first_page = other.first_page;
          current_page = other.current_page;
          free_list = other.free_list;
          block_size = other.block_size;
          blocks_per_page = other.blocks_per_page;
          page_size = other.page_size;
          total_allocations = other.total_allocations;
          active_allocations = other.active_allocations;
          page_count = other.page_count;

          other.first_page = nullptr;
          other.current_page = nullptr;
          other.free_list = nullptr;
        }
        return *this;
      }

      /**
       * @brief Allocates a single block of memory.
       *
       * Time Complexity: O(1) - pops from free list
       *
       * @return Pointer to the allocated block, or nullptr if allocation fails.
       */
      void* allocate() {
        // If free list is empty, try to add a new page
        if (!free_list) {
          if (!add_page()) {
            return nullptr;  // Out of memory
          }
        }

        // Pop from free list
        FreeListNode* node = free_list;
        free_list = free_list->next;

        // Update statistics
        ++total_allocations;
        ++active_allocations;

        return static_cast<void*>(node);
      }

      /**
       * @brief Deallocates a block of memory.
       *
       * Time Complexity: O(1) - pushes to free list
       *
       * @param ptr Pointer to the block to deallocate.
       *
       * @note The memory is NOT zeroed or returned to the OS.
       *       It is simply added back to the free list for reuse.
       */
      void deallocate(void* ptr) {
        if (!ptr) return;

        // Push to free list
        FreeListNode* node = static_cast<FreeListNode*>(ptr);
        node->next = free_list;
        free_list = node;

        // Update statistics
        --active_allocations;
      }

      /**
       * @brief Constructs an object of type T in allocated memory.
       *
       * Uses placement new to construct the object in pre-allocated memory.
       *
       * @tparam T The type of object to construct.
       * @tparam Args Constructor argument types.
       * @param args Arguments to forward to T's constructor.
       * @return Pointer to the constructed object, or nullptr if allocation fails.
       */
      template<typename T, typename... Args>
      T* construct(Args&&... args) {
        static_assert(sizeof(T) <= block_size || block_size >= sizeof(T),
          "Object size exceeds pool block size");

        void* memory = allocate();
        if (!memory) return nullptr;

        // Use placement new to construct object in pre-allocated memory
        return new (memory) T(std::forward<Args>(args)...);
      }

      /**
       * @brief Destructs an object and returns its memory to the pool.
       *
       * Calls the object's destructor, then adds the memory back to the free list.
       *
       * @tparam T The type of object to destruct.
       * @param ptr Pointer to the object to destruct.
       */
      template<typename T>
      void destruct(T* ptr) {
        if (!ptr) return;

        // Call destructor explicitly
        ptr->~T();

        // Return memory to pool
        deallocate(ptr);
      }

      // ============================================================================
      // Statistics and Debugging
      // ============================================================================

      /// Returns the size of each block in bytes
      size_t get_block_size() const { return block_size; }

      /// Returns the number of blocks per page
      size_t get_blocks_per_page() const { return blocks_per_page; }

      /// Returns the total number of allocations made
      size_t get_total_allocations() const { return total_allocations; }

      /// Returns the number of currently active allocations
      size_t get_active_allocations() const { return active_allocations; }

      /// Returns the number of pages allocated
      size_t get_page_count() const { return page_count; }

      /// Returns the total memory allocated (all pages)
      size_t get_total_memory() const { return page_count * page_size; }

      /// Returns the memory currently in use
      size_t get_used_memory() const { return active_allocations * block_size; }

      /**
       * @brief Resets the allocator, returning all blocks to the free list.
       *
       * This is useful when unloading a level - all objects are considered freed.
       * The memory pages themselves are NOT deallocated.
       */
      void reset() {
        free_list = nullptr;
        active_allocations = 0;

        // Re-initialize all pages
        MemoryPage* page = first_page;
        while (page) {
          initialize_page_blocks(page);
          page = page->next_page;
        }
      }

      /**
       * @brief Pre-allocates additional pages.
       *
       * Call this during level load to ensure enough memory is available.
       *
       * @param num_pages Number of pages to pre-allocate.
       */
      void reserve_pages(size_t num_pages) {
        for (size_t i = 0; i < num_pages; ++i) {
          add_page();
        }
      }
    };

    // ============================================================================
    // TypedPoolAllocator - Type-safe wrapper around PoolAllocator
    // ============================================================================

    /**
     * @brief Type-safe pool allocator for a specific type T.
     *
     * This is a convenience wrapper that provides type-safe allocation
     * and construction methods.
     *
     * @tparam T The type of objects this pool allocates.
     */
    template<typename T>
    class TypedPoolAllocator {
    private:
      PoolAllocator pool;

    public:
      /**
       * @brief Constructs a typed pool allocator.
       * @param initial_page_size Size of initial memory page.
       * @param alignment Memory alignment for blocks.
       */
      TypedPoolAllocator(size_t initial_page_size = DEFAULT_PAGE_SIZE,
        size_t alignment = DEFAULT_ALIGNMENT)
        : pool(sizeof(T), initial_page_size, alignment)
      {
      }

      /**
       * @brief Allocates and constructs an object of type T.
       * @tparam Args Constructor argument types.
       * @param args Arguments to forward to T's constructor.
       * @return Pointer to the constructed object.
       */
      template<typename... Args>
      T* construct(Args&&... args) {
        return pool.construct<T>(std::forward<Args>(args)...);
      }

      /**
       * @brief Destructs an object and returns its memory to the pool.
       * @param ptr Pointer to the object to destruct.
       */
      void destruct(T* ptr) {
        pool.destruct<T>(ptr);
      }

      /// Returns the underlying pool allocator
      PoolAllocator& get_pool() { return pool; }
      const PoolAllocator& get_pool() const { return pool; }

      // Forward statistics methods
      size_t get_total_allocations() const { return pool.get_total_allocations(); }
      size_t get_active_allocations() const { return pool.get_active_allocations(); }
      size_t get_page_count() const { return pool.get_page_count(); }
      size_t get_total_memory() const { return pool.get_total_memory(); }
      size_t get_used_memory() const { return pool.get_used_memory(); }

      void reset() { pool.reset(); }
      void reserve_pages(size_t num_pages) { pool.reserve_pages(num_pages); }
    };

    // ============================================================================
    // MemoryManager - Global manager for all memory pools
    // ============================================================================

    /**
     * @brief Global memory manager singleton.
     *
     * Provides centralized access to memory pools for different object types.
     * This class manages component pools and entity pools.
     */
    class MemoryManager {
    private:
      // We use a simple fixed-size array to store pool allocators
      // This avoids STL containers in the memory manager itself
      static constexpr size_t MAX_POOLS = 128;

      struct PoolEntry {
        PoolAllocator* pool;
        size_t type_hash;  // For type identification
        bool active;
      };

      PoolEntry pools[MAX_POOLS];
      size_t pool_count;

      // Private constructor for singleton
      MemoryManager() : pool_count(0) {
        for (size_t i = 0; i < MAX_POOLS; ++i) {
          pools[i].pool = nullptr;
          pools[i].type_hash = 0;
          pools[i].active = false;
        }
      }

    public:
      /// Gets the singleton instance
      static MemoryManager& instance() {
        static MemoryManager manager;
        return manager;
      }

      // Disable copy/move
      MemoryManager(const MemoryManager&) = delete;
      MemoryManager& operator=(const MemoryManager&) = delete;

      /**
       * @brief Gets or creates a pool allocator for type T.
       * @tparam T The type to get a pool for.
       * @param page_size Size of pages for the pool.
       * @return Reference to the pool allocator.
       */
      template<typename T>
      PoolAllocator& get_pool(size_t page_size = DEFAULT_PAGE_SIZE) {
        // Simple type hash using typeid
        size_t type_hash = typeid(T).hash_code();

        // Look for existing pool
        for (size_t i = 0; i < pool_count; ++i) {
          if (pools[i].active && pools[i].type_hash == type_hash) {
            return *pools[i].pool;
          }
        }

        // Create new pool
        if (pool_count < MAX_POOLS) {
          pools[pool_count].pool = new PoolAllocator(sizeof(T), page_size);
          pools[pool_count].type_hash = type_hash;
          pools[pool_count].active = true;
          return *pools[pool_count++].pool;
        }

        // Fallback - this should never happen in practice
        static PoolAllocator fallback(sizeof(T), page_size);
        return fallback;
      }

      /**
       * @brief Allocates and constructs an object of type T.
       * @tparam T The type to allocate.
       * @tparam Args Constructor argument types.
       * @param args Arguments to forward to T's constructor.
       * @return Pointer to the constructed object.
       */
      template<typename T, typename... Args>
      T* allocate(Args&&... args) {
        return get_pool<T>().construct<T>(std::forward<Args>(args)...);
      }

      /**
       * @brief Deallocates an object of type T.
       * @tparam T The type of object.
       * @param ptr Pointer to the object to deallocate.
       */
      template<typename T>
      void deallocate(T* ptr) {
        get_pool<T>().destruct<T>(ptr);
      }

      /**
       * @brief Resets all pools, returning all memory to free lists.
       *
       * Call this when unloading a level.
       */
      void reset_all() {
        for (size_t i = 0; i < pool_count; ++i) {
          if (pools[i].active && pools[i].pool) {
            pools[i].pool->reset();
          }
        }
      }

      /**
       * @brief Gets total memory used across all pools.
       * @return Total memory in bytes.
       */
      size_t get_total_memory() const {
        size_t total = 0;
        for (size_t i = 0; i < pool_count; ++i) {
          if (pools[i].active && pools[i].pool) {
            total += pools[i].pool->get_total_memory();
          }
        }
        return total;
      }

      /**
       * @brief Gets total active allocations across all pools.
       * @return Total active allocations.
       */
      size_t get_total_active_allocations() const {
        size_t total = 0;
        for (size_t i = 0; i < pool_count; ++i) {
          if (pools[i].active && pools[i].pool) {
            total += pools[i].pool->get_active_allocations();
          }
        }
        return total;
      }

      /// Destructor - cleans up all pools
      ~MemoryManager() {
        for (size_t i = 0; i < pool_count; ++i) {
          if (pools[i].pool) {
            delete pools[i].pool;
            pools[i].pool = nullptr;
          }
        }
      }
    };

    // ============================================================================
    // Convenience Macros
    // ============================================================================

    /// Allocate an object using the memory manager
#define POOL_NEW(Type, ...) \
      Engine::Memory::MemoryManager::instance().allocate<Type>(__VA_ARGS__)

    /// Deallocate an object using the memory manager
#define POOL_DELETE(Type, ptr) \
      Engine::Memory::MemoryManager::instance().deallocate<Type>(ptr)

  } // namespace Memory
} // namespace Engine
