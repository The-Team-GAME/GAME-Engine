//-----------------------------------------------------------------------------------
// file:    MemoryPage.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Defines the MemoryPage class which represents a contiguous block of memory
//          divided into fixed-size blocks. Uses an intrusive free list for O(1)
//          allocation and deallocation. No STL containers are used - the free list
//          is stored directly within the free blocks themselves.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "MemoryConfig.h"

namespace Engine {
    namespace Memory {

        //=================================================================================
        // MemoryPage - A single page of memory containing fixed-size blocks
        //
        // Memory Layout:
        // [Block 0][Block 1][Block 2]...[Block N-1]
        //
        // Free List (intrusive):
        // Each free block stores a pointer to the next free block at its beginning.
        // This means each block must be at least sizeof(void*) bytes.
        //
        // When a block is allocated, it is removed from the free list.
        // When a block is freed, it is pushed back onto the free list.
        // No actual memory is freed until the entire page is destroyed.
        //=================================================================================
        class MemoryPage {
        public:
            //=================================================================================
            // Constructors / Destructor
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Default constructor - creates an uninitialized page.

            Parameters:
            None.

            Where to call it:
            When declaring a MemoryPage that will be initialized later.
            ==================================================================================*/
            MemoryPage();

            /*=================================================================================
            Function Brief:
            Constructs and initializes a memory page with the specified block size and count.
            Allocates raw memory from the OS and builds the free list.

            Parameters:
            block_size - Size of each block in bytes (must be >= MIN_BLOCK_SIZE).
            block_count - Number of blocks in this page.

            Where to call it:
            When creating a new pool or when a pool needs to expand.
            ==================================================================================*/
            MemoryPage(size_t block_size, size_t block_count);

            /*=================================================================================
            Function Brief:
            Destructor - releases the raw memory back to the OS.

            Parameters:
            None.

            Where to call it:
            Automatically called when the page is destroyed.
            ==================================================================================*/
            ~MemoryPage();

            // Disable copy to prevent double-free
            MemoryPage(const MemoryPage&) = delete;
            MemoryPage& operator=(const MemoryPage&) = delete;

            // Enable move semantics
            MemoryPage(MemoryPage&& other) noexcept;
            MemoryPage& operator=(MemoryPage&& other) noexcept;

            //=================================================================================
            // Initialization
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Initializes the memory page with the specified block size and count.
            Can only be called on an uninitialized page.

            Parameters:
            block_size - Size of each block in bytes.
            block_count - Number of blocks in this page.

            Returns:
            true if initialization succeeded, false if already initialized or allocation failed.

            Where to call it:
            After default construction, before using the page.
            ==================================================================================*/
            bool initialize(size_t block_size, size_t block_count);

            /*=================================================================================
            Function Brief:
            Shuts down the page and releases all memory.

            Parameters:
            None.

            Where to call it:
            When the page is no longer needed but not being destroyed.
            ==================================================================================*/
            void shutdown();

            //=================================================================================
            // Allocation / Deallocation
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Allocates a single block from this page. Returns nullptr if no blocks are free.
            Uses the intrusive free list for O(1) allocation.

            Parameters:
            None.

            Returns:
            Pointer to the allocated block, or nullptr if the page is full.

            Where to call it:
            When the pool allocator needs a block from this page.
            ==================================================================================*/
            void* allocate();

            /*=================================================================================
            Function Brief:
            Deallocates a block back to this page. The block must belong to this page.
            The memory is not returned to the OS - it's added back to the free list.
            Uses the intrusive free list for O(1) deallocation.

            Parameters:
            ptr - Pointer to the block to deallocate.

            Where to call it:
            When the pool allocator returns a block to this page.
            ==================================================================================*/
            void deallocate(void* ptr);

            //=================================================================================
            // Queries
            //=================================================================================

            /*=================================================================================
            Function Brief:
            Checks if a pointer belongs to this page's memory range.

            Parameters:
            ptr - The pointer to check.

            Returns:
            true if the pointer is within this page's memory range.

            Where to call it:
            When determining which page owns a given pointer.
            ==================================================================================*/
            bool owns(const void* ptr) const;

            /*=================================================================================
            Function Brief:
            Returns true if the page has any free blocks available.

            Parameters:
            None.

            Returns:
            true if there are free blocks, false if the page is full.

            Where to call it:
            When checking if a page can satisfy an allocation request.
            ==================================================================================*/
            bool has_free_blocks() const;

            /*=================================================================================
            Function Brief:
            Returns the number of free blocks remaining in this page.

            Parameters:
            None.

            Returns:
            Number of free blocks.

            Where to call it:
            For debugging or statistics.
            ==================================================================================*/
            size_t get_free_count() const;

            /*=================================================================================
            Function Brief:
            Returns the total number of blocks in this page.

            Parameters:
            None.

            Returns:
            Total block count.

            Where to call it:
            For debugging or statistics.
            ==================================================================================*/
            size_t get_block_count() const;

            /*=================================================================================
            Function Brief:
            Returns the size of each block in this page.

            Parameters:
            None.

            Returns:
            Block size in bytes.

            Where to call it:
            When verifying block sizes match.
            ==================================================================================*/
            size_t get_block_size() const;

            /*=================================================================================
            Function Brief:
            Returns true if the page has been initialized.

            Parameters:
            None.

            Returns:
            true if initialized, false otherwise.

            Where to call it:
            Before using the page.
            ==================================================================================*/
            bool is_initialized() const;

            //=================================================================================
            // Linked List Support
            //=================================================================================

            // Pointer to next page (for creating linked lists of pages in pool allocators)
            // Not owned by this page - just a link.
            MemoryPage* next_page;

        private:
            void* m_memory;           // Raw memory block from OS
            void* m_free_list_head;   // Head of intrusive free list
            size_t m_block_size;      // Size of each block
            size_t m_block_count;     // Total number of blocks
            size_t m_free_count;      // Number of free blocks
            void* m_memory_end;       // End of memory range (for bounds checking)

            /*=================================================================================
            Function Brief:
            Builds the intrusive free list by linking all blocks together.

            Parameters:
            None.

            Where to call it:
            During initialization after memory is allocated.
            ==================================================================================*/
            void build_free_list();
        };

    } // namespace Memory
} // namespace Engine
