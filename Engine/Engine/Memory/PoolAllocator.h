//-----------------------------------------------------------------------------------
// file:    PoolAllocator.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Defines the PoolAllocator class which manages a pool of fixed-size memory
//          blocks. Uses a linked list of MemoryPages for dynamic expansion when the
//          pool runs out of space. Provides O(1) allocation and deallocation.
//          No STL containers are used - pages are linked using raw pointers.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "MemoryPage.h"

namespace Engine {
    namespace Memory {

        //=================================================================================
        // PoolAllocator - Manages multiple pages of fixed-size blocks
        //
        // The pool maintains a linked list of MemoryPages. When a page runs out of
        // blocks, a new page is automatically added. This allows the pool to grow
        // dynamically while minimizing OS allocations (only when expanding).
        //
        // Page Selection Strategy:
        // - Maintains a "current page" pointer for fast allocation
        // - When current page is full, moves to next page with free blocks
        // - If no pages have free blocks, allocates a new page
        //=================================================================================
        class PoolAllocator {
        public:
            //=================================================================================
            // Constructors / Destructor
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Default constructor - creates an uninitialized pool.

            Parameters:
            None.

            Where to call it:
            When declaring a PoolAllocator that will be initialized later.
            ==================================================================================*/
            PoolAllocator();

            /*=================================================================================
            Function Brief:
            Constructs and initializes a pool with the specified block size and initial count.

            Parameters:
            block_size - Size of each block in bytes.
            initial_block_count - Initial number of blocks to allocate.

            Where to call it:
            At level/game load time to pre-allocate memory.
            ==================================================================================*/
            PoolAllocator(size_t block_size, size_t initial_block_count);

            /*=================================================================================
            Function Brief:
            Destructor - releases all pages back to the OS.

            Parameters:
            None.

            Where to call it:
            Automatically called when the pool is destroyed.
            ==================================================================================*/
            ~PoolAllocator();

            // Disable copy
            PoolAllocator(const PoolAllocator&) = delete;
            PoolAllocator& operator=(const PoolAllocator&) = delete;

            // Enable move
            PoolAllocator(PoolAllocator&& other) noexcept;
            PoolAllocator& operator=(PoolAllocator&& other) noexcept;

            //=================================================================================
            // Initialization
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Initializes the pool with the specified block size and initial count.

            Parameters:
            block_size - Size of each block in bytes.
            initial_block_count - Initial number of blocks to allocate.

            Returns:
            true if initialization succeeded, false otherwise.

            Where to call it:
            After default construction, at level load time.
            ==================================================================================*/
            bool initialize(size_t block_size, size_t initial_block_count);

            /*=================================================================================
            Function Brief:
            Shuts down the pool and releases all memory.

            Parameters:
            None.

            Where to call it:
            At level unload time or when the pool is no longer needed.
            ==================================================================================*/
            void shutdown();

            //=================================================================================
            // Allocation / Deallocation
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Allocates a single block from the pool. If no blocks are available, a new
            page is added automatically. Returns nullptr only if OS allocation fails.

            Parameters:
            None.

            Returns:
            Pointer to the allocated block, or nullptr if allocation failed.

            Where to call it:
            When an object needs memory from this pool.
            ==================================================================================*/
            void* allocate();

            /*=================================================================================
            Function Brief:
            Deallocates a block back to the pool. The block is returned to its
            owning page's free list. The memory is NOT returned to the OS.

            Parameters:
            ptr - Pointer to the block to deallocate.

            Where to call it:
            When an object's memory should be returned to the pool.
            ==================================================================================*/
            void deallocate(void* ptr);

            //=================================================================================
            // Queries
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Returns the block size for this pool.

            Parameters:
            None.

            Returns:
            Block size in bytes.

            Where to call it:
            For debugging or verification.
            ==================================================================================*/
            size_t get_block_size() const;

            /*=================================================================================
            Function Brief:
            Returns the total number of blocks across all pages.

            Parameters:
            None.

            Returns:
            Total block count.

            Where to call it:
            For statistics or debugging.
            ==================================================================================*/
            size_t get_total_blocks() const;

            /*=================================================================================
            Function Brief:
            Returns the number of free blocks across all pages.

            Parameters:
            None.

            Returns:
            Free block count.

            Where to call it:
            For statistics or debugging.
            ==================================================================================*/
            size_t get_free_blocks() const;

            /*=================================================================================
            Function Brief:
            Returns the number of allocated (in-use) blocks.

            Parameters:
            None.

            Returns:
            Allocated block count.

            Where to call it:
            For statistics or debugging.
            ==================================================================================*/
            size_t get_allocated_blocks() const;

            /*=================================================================================
            Function Brief:
            Returns the number of pages in this pool.

            Parameters:
            None.

            Returns:
            Page count.

            Where to call it:
            For statistics or debugging.
            ==================================================================================*/
            size_t get_page_count() const;

            /*=================================================================================
            Function Brief:
            Returns true if the pool has been initialized.

            Parameters:
            None.

            Returns:
            true if initialized, false otherwise.

            Where to call it:
            Before using the pool.
            ==================================================================================*/
            bool is_initialized() const;

            //=================================================================================
            // Debug / Maintenance
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Resets the pool - deallocates all blocks but keeps the pages.
            Useful for level transitions.

            Parameters:
            None.

            Where to call it:
            When transitioning between levels to reset all allocations.
            ==================================================================================*/
            void reset();

        private:
            MemoryPage* m_pages;          // Linked list of all pages (head)
            MemoryPage* m_current_page;   // Current page for allocation (for fast path)
            size_t m_block_size;          // Size of each block
            size_t m_blocks_per_page;     // Number of blocks per page
            size_t m_page_count;          // Total number of pages

            /*=================================================================================
            Function Brief:
            Adds a new page to the pool. Called automatically when out of blocks.

            Parameters:
            None.

            Returns:
            true if the page was added successfully, false if OS allocation failed.

            Where to call it:
            Internally when the current page is full.
            ==================================================================================*/
            bool add_page();

            /*=================================================================================
            Function Brief:
            Finds a page with free blocks. Used when current page is exhausted.

            Parameters:
            None.

            Returns:
            Pointer to a page with free blocks, or nullptr if all pages are full.

            Where to call it:
            Internally during allocation when current page is full.
            ==================================================================================*/
            MemoryPage* find_page_with_free_blocks();
        };

    } // namespace Memory
} // namespace Engine
