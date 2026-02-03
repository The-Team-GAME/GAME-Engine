//-----------------------------------------------------------------------------------
// file:    MemoryPage.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the MemoryPage class. Handles raw memory allocation
//          from the OS and manages an intrusive free list for O(1) block allocation
//          and deallocation. Uses placement of pointers within free blocks to avoid
//          any additional memory overhead.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "MemoryPage.h"
#include <new>      // For ::operator new and ::operator delete
#include <cstring>  // For memset (optional, for debugging)

namespace Engine {
    namespace Memory {

        //=================================================================================
        // Constructors / Destructor
        //=================================================================================

        MemoryPage::MemoryPage()
            : m_memory(nullptr)
            , m_free_list_head(nullptr)
            , m_block_size(0)
            , m_block_count(0)
            , m_free_count(0)
            , m_memory_end(nullptr)
            , next_page(nullptr)
        {
        }

        MemoryPage::MemoryPage(size_t block_size, size_t block_count)
            : m_memory(nullptr)
            , m_free_list_head(nullptr)
            , m_block_size(0)
            , m_block_count(0)
            , m_free_count(0)
            , m_memory_end(nullptr)
            , next_page(nullptr)
        {
            initialize(block_size, block_count);
        }

        MemoryPage::~MemoryPage() {
            shutdown();
        }

        MemoryPage::MemoryPage(MemoryPage&& other) noexcept
            : m_memory(other.m_memory)
            , m_free_list_head(other.m_free_list_head)
            , m_block_size(other.m_block_size)
            , m_block_count(other.m_block_count)
            , m_free_count(other.m_free_count)
            , m_memory_end(other.m_memory_end)
            , next_page(other.next_page)
        {
            // Reset the source object
            other.m_memory = nullptr;
            other.m_free_list_head = nullptr;
            other.m_block_size = 0;
            other.m_block_count = 0;
            other.m_free_count = 0;
            other.m_memory_end = nullptr;
            other.next_page = nullptr;
        }

        MemoryPage& MemoryPage::operator=(MemoryPage&& other) noexcept {
            if (this != &other) {
                // Clean up existing memory
                shutdown();

                // Move from other
                m_memory = other.m_memory;
                m_free_list_head = other.m_free_list_head;
                m_block_size = other.m_block_size;
                m_block_count = other.m_block_count;
                m_free_count = other.m_free_count;
                m_memory_end = other.m_memory_end;
                next_page = other.next_page;

                // Reset the source object
                other.m_memory = nullptr;
                other.m_free_list_head = nullptr;
                other.m_block_size = 0;
                other.m_block_count = 0;
                other.m_free_count = 0;
                other.m_memory_end = nullptr;
                other.next_page = nullptr;
            }
            return *this;
        }

        //=================================================================================
        // Initialization
        //=================================================================================

        bool MemoryPage::initialize(size_t block_size, size_t block_count) {
            // Don't reinitialize
            if (m_memory != nullptr) {
                return false;
            }

            // Enforce minimum block size (must hold a pointer for the free list)
            if (block_size < sizeof(void*)) {
                block_size = sizeof(void*);
            }

            // Ensure block size is at least MIN_BLOCK_SIZE for alignment
            if (block_size < MIN_BLOCK_SIZE) {
                block_size = MIN_BLOCK_SIZE;
            }

            // Align block size to DEFAULT_ALIGNMENT
            block_size = align_address(block_size, DEFAULT_ALIGNMENT);

            // Calculate total memory needed
            size_t total_size = block_size * block_count;

            // Allocate raw memory from OS
            // This is the ONLY OS allocation - happens at load time!
            m_memory = ::operator new(total_size, std::nothrow);
            if (!m_memory) {
                return false;
            }

            m_block_size = block_size;
            m_block_count = block_count;
            m_free_count = block_count;
            m_memory_end = static_cast<char*>(m_memory) + total_size;

            // Build the intrusive free list
            build_free_list();

            return true;
        }

        void MemoryPage::shutdown() {
            if (m_memory) {
                // Return memory to OS
                ::operator delete(m_memory);
                m_memory = nullptr;
                m_free_list_head = nullptr;
                m_block_size = 0;
                m_block_count = 0;
                m_free_count = 0;
                m_memory_end = nullptr;
            }
        }

        //=================================================================================
        // Allocation / Deallocation
        //=================================================================================

        void* MemoryPage::allocate() {
            // Check if we have any free blocks
            if (!m_free_list_head) {
                return nullptr;
            }

            // Pop the first block from the free list
            // The free block contains a pointer to the next free block at its start
            void* block = m_free_list_head;

            // The next free block pointer is stored at the beginning of the current block
            m_free_list_head = *static_cast<void**>(m_free_list_head);

            --m_free_count;

            return block;
        }

        void MemoryPage::deallocate(void* ptr) {
            // Verify the pointer belongs to this page
            if (!owns(ptr)) {
                return; // Invalid pointer - ignore or assert in debug
            }

            // Push the block back onto the free list
            // Store the current head pointer at the beginning of the freed block
            *static_cast<void**>(ptr) = m_free_list_head;
            m_free_list_head = ptr;

            ++m_free_count;
        }

        //=================================================================================
        // Queries
        //=================================================================================

        bool MemoryPage::owns(const void* ptr) const {
            return ptr >= m_memory && ptr < m_memory_end;
        }

        bool MemoryPage::has_free_blocks() const {
            return m_free_list_head != nullptr;
        }

        size_t MemoryPage::get_free_count() const {
            return m_free_count;
        }

        size_t MemoryPage::get_block_count() const {
            return m_block_count;
        }

        size_t MemoryPage::get_block_size() const {
            return m_block_size;
        }

        bool MemoryPage::is_initialized() const {
            return m_memory != nullptr;
        }

        //=================================================================================
        // Private Methods
        //=================================================================================

        void MemoryPage::build_free_list() {
            // Build the free list by linking all blocks together
            // Each block stores a pointer to the next block at its beginning

            m_free_list_head = m_memory;

            char* current = static_cast<char*>(m_memory);

            // Link each block to the next
            for (size_t i = 0; i < m_block_count - 1; ++i) {
                // Store pointer to next block in current block
                void* next_block = current + m_block_size;
                *reinterpret_cast<void**>(current) = next_block;
                current += m_block_size;
            }

            // Last block points to nullptr (end of list)
            *reinterpret_cast<void**>(current) = nullptr;
        }

    } // namespace Memory
} // namespace Engine
