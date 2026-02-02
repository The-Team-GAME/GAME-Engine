//-----------------------------------------------------------------------------------
// file:    PoolAllocator.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the PoolAllocator class. Manages a linked list of
//          MemoryPages for efficient fixed-size block allocation. Dynamically
//          expands by adding new pages when needed. No STL containers used.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "PoolAllocator.h"
#include "../Logger.h"
#include <new> // For ::operator new and ::operator delete

namespace Engine {
    namespace Memory {

        //=================================================================================
        // Constructors / Destructor
        //=================================================================================

        PoolAllocator::PoolAllocator()
            : m_pages(nullptr)
            , m_current_page(nullptr)
            , m_block_size(0)
            , m_blocks_per_page(0)
            , m_page_count(0)
        {
        }

        PoolAllocator::PoolAllocator(size_t block_size, size_t initial_block_count)
            : m_pages(nullptr)
            , m_current_page(nullptr)
            , m_block_size(0)
            , m_blocks_per_page(0)
            , m_page_count(0)
        {
            initialize(block_size, initial_block_count);
        }

        PoolAllocator::~PoolAllocator() {
            shutdown();
        }

        PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
            : m_pages(other.m_pages)
            , m_current_page(other.m_current_page)
            , m_block_size(other.m_block_size)
            , m_blocks_per_page(other.m_blocks_per_page)
            , m_page_count(other.m_page_count)
        {
            other.m_pages = nullptr;
            other.m_current_page = nullptr;
            other.m_block_size = 0;
            other.m_blocks_per_page = 0;
            other.m_page_count = 0;
        }

        PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
            if (this != &other) {
                shutdown();

                m_pages = other.m_pages;
                m_current_page = other.m_current_page;
                m_block_size = other.m_block_size;
                m_blocks_per_page = other.m_blocks_per_page;
                m_page_count = other.m_page_count;

                other.m_pages = nullptr;
                other.m_current_page = nullptr;
                other.m_block_size = 0;
                other.m_blocks_per_page = 0;
                other.m_page_count = 0;
            }
            return *this;
        }

        //=================================================================================
        // Initialization
        //=================================================================================

        bool PoolAllocator::initialize(size_t block_size, size_t initial_block_count) {
            if (m_pages != nullptr) {
                return false; // Already initialized
            }

            // Ensure minimum block size
            if (block_size < MIN_BLOCK_SIZE) {
                block_size = MIN_BLOCK_SIZE;
            }

            // Align block size
            m_block_size = align_address(block_size, DEFAULT_ALIGNMENT);
            m_blocks_per_page = initial_block_count;

            // Create the first page
            return add_page();
        }

        void PoolAllocator::shutdown() {
            // Walk the linked list and delete all pages
            MemoryPage* page = m_pages;
            while (page != nullptr) {
                MemoryPage* next = page->next_page;

                // Destroy the page
                page->~MemoryPage();
                ::operator delete(page);

                page = next;
            }

            m_pages = nullptr;
            m_current_page = nullptr;
            m_block_size = 0;
            m_blocks_per_page = 0;
            m_page_count = 0;
        }

        //=================================================================================
        // Allocation / Deallocation
        //=================================================================================

        void* PoolAllocator::allocate() {
            // Fast path: try current page first
            if (m_current_page && m_current_page->has_free_blocks()) {
                return m_current_page->allocate();
            }

            // Slow path: find a page with free blocks
            m_current_page = find_page_with_free_blocks();

            if (m_current_page) {
                return m_current_page->allocate();
            }

            // No free blocks anywhere - need to add a new page
            // This is the only time we allocate from OS at runtime!
            if (!add_page()) {
                return nullptr; // OS allocation failed
            }

            // The new page becomes current and should have free blocks
            return m_current_page->allocate();
        }

        void PoolAllocator::deallocate(void* ptr) {
            if (!ptr) return;

            // Find the page that owns this pointer
            MemoryPage* page = m_pages;
            while (page != nullptr) {
                if (page->owns(ptr)) {
                    page->deallocate(ptr);

                    // Update current page hint if this page now has free blocks
                    // and current page is full
                    if (!m_current_page || !m_current_page->has_free_blocks()) {
                        m_current_page = page;
                    }
                    return;
                }
                page = page->next_page;
            }

            // Pointer doesn't belong to any page - ignore (or assert in debug)
        }

        //=================================================================================
        // Queries
        //=================================================================================

        size_t PoolAllocator::get_block_size() const {
            return m_block_size;
        }

        size_t PoolAllocator::get_total_blocks() const {
            size_t total = 0;
            MemoryPage* page = m_pages;
            while (page != nullptr) {
                total += page->get_block_count();
                page = page->next_page;
            }
            return total;
        }

        size_t PoolAllocator::get_free_blocks() const {
            size_t free = 0;
            MemoryPage* page = m_pages;
            while (page != nullptr) {
                free += page->get_free_count();
                page = page->next_page;
            }
            return free;
        }

        size_t PoolAllocator::get_allocated_blocks() const {
            return get_total_blocks() - get_free_blocks();
        }

        size_t PoolAllocator::get_page_count() const {
            return m_page_count;
        }

        bool PoolAllocator::is_initialized() const {
            return m_pages != nullptr;
        }

        //=================================================================================
        // Debug / Maintenance
        //=================================================================================

        void PoolAllocator::reset() {
            // Walk all pages and reset their free lists
            // This is more efficient than deallocating each block individually
            MemoryPage* page = m_pages;
            while (page != nullptr) {
                // Shutdown and reinitialize each page
                size_t block_count = page->get_block_count();
                size_t block_size = page->get_block_size();
                page->shutdown();
                page->initialize(block_size, block_count);
                page = page->next_page;
            }

            // Reset current page to head
            m_current_page = m_pages;
        }

        //=================================================================================
        // Private Methods
        //=================================================================================

        bool PoolAllocator::add_page() {
            // Allocate memory for the page struct itself
            void* page_memory = ::operator new(sizeof(MemoryPage), std::nothrow);
            if (!page_memory) {
                Log::memory("POOL EXPANSION FAILED: Could not allocate page struct for {} byte blocks", m_block_size);
                return false;
            }

            // Construct the page in place
            MemoryPage* new_page = new (page_memory) MemoryPage();

            // Initialize the page with blocks
            if (!new_page->initialize(m_block_size, m_blocks_per_page)) {
                Log::memory("POOL EXPANSION FAILED: Could not initialize page for {} byte blocks", m_block_size);
                new_page->~MemoryPage();
                ::operator delete(page_memory);
                return false;
            }

            // Link the new page to the front of the list
            new_page->next_page = m_pages;
            m_pages = new_page;
            m_current_page = new_page;
            ++m_page_count;

            // Log the dynamic expansion - this is important to track!
            size_t page_bytes = m_block_size * m_blocks_per_page;
            Log::memory("POOL EXPANSION: Added page #{} for {} byte blocks ({} blocks = {} KB) - OS allocation at runtime!",
                m_page_count, m_block_size, m_blocks_per_page, page_bytes / 1024);

            return true;
        }

        MemoryPage* PoolAllocator::find_page_with_free_blocks() {
            MemoryPage* page = m_pages;
            while (page != nullptr) {
                if (page->has_free_blocks()) {
                    return page;
                }
                page = page->next_page;
            }
            return nullptr;
        }

    } // namespace Memory
} // namespace Engine
