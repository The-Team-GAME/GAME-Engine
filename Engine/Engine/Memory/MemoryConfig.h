//-----------------------------------------------------------------------------------
// file:    MemoryConfig.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Configuration constants and type definitions for the custom Memory Manager.
//          Defines page sizes, pool configurations, and memory alignment requirements.
//          All memory allocations in the ECS use these settings for consistency.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../TypeDef.h"

namespace Engine {
    namespace Memory {

        //=================================================================================
        // Memory Configuration Constants
        // These values can be tuned based on your game's requirements
        //=================================================================================

        // Default page size for memory allocation (4KB - typical OS page size)
        inline constexpr size_t DEFAULT_PAGE_SIZE = 4096;

        // Large page size for bulk allocations (64KB)
        inline constexpr size_t LARGE_PAGE_SIZE = 65536;

        // Maximum number of pool allocators for different size classes
        // Covers sizes: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768
        inline constexpr size_t NUM_SIZE_CLASSES = 12;

        // Minimum block size (16 bytes - ensures alignment and space for free list pointer)
        inline constexpr size_t MIN_BLOCK_SIZE = 16;

        // Default alignment for memory allocations
        inline constexpr size_t DEFAULT_ALIGNMENT = 16;

        // Initial number of blocks per pool (can grow dynamically)
        inline constexpr size_t DEFAULT_BLOCKS_PER_PAGE = 256;

        // Maximum entity count hint for pre-allocation
        inline constexpr size_t DEFAULT_MAX_ENTITIES = 10000;

        // Maximum component types hint for pre-allocation
        inline constexpr size_t DEFAULT_MAX_COMPONENT_TYPES = 64;

        // Null index marker for sparse arrays and free lists
        inline constexpr size_t NULL_INDEX = static_cast<size_t>(-1);

        //=================================================================================
        // Size Class Utilities
        //=================================================================================

        /*=================================================================================
        Function Brief:
        Calculates the size class index for a given allocation size.
        Size classes are powers of 2 starting from MIN_BLOCK_SIZE.

        Parameters:
        size - The requested allocation size in bytes.

        Returns:
        The index of the appropriate size class pool (0 to NUM_SIZE_CLASSES-1).
        Returns NUM_SIZE_CLASSES if size exceeds maximum pool size.
        ==================================================================================*/
        constexpr size_t get_size_class_index(size_t size) {
            if (size <= MIN_BLOCK_SIZE) return 0;

            // Find the next power of 2 >= size
            size_t block_size = MIN_BLOCK_SIZE;
            size_t index = 0;
            while (block_size < size && index < NUM_SIZE_CLASSES - 1) {
                block_size *= 2;
                ++index;
            }
            return index;
        }

        /*=================================================================================
        Function Brief:
        Returns the actual block size for a given size class index.

        Parameters:
        index - The size class index (0 to NUM_SIZE_CLASSES-1).

        Returns:
        The block size in bytes for that size class.
        ==================================================================================*/
        constexpr size_t get_block_size_for_class(size_t index) {
            return MIN_BLOCK_SIZE << index; // MIN_BLOCK_SIZE * 2^index
        }

        /*=================================================================================
        Function Brief:
        Rounds up a size to the nearest valid block size (power of 2 >= MIN_BLOCK_SIZE).

        Parameters:
        size - The requested size in bytes.

        Returns:
        The rounded-up block size.
        ==================================================================================*/
        constexpr size_t round_up_to_block_size(size_t size) {
            size_t index = get_size_class_index(size);
            return get_block_size_for_class(index);
        }

        /*=================================================================================
        Function Brief:
        Checks if a size can be handled by a pool allocator (vs. direct OS allocation).

        Parameters:
        size - The requested size in bytes.

        Returns:
        true if the size can be handled by a pool, false otherwise.
        ==================================================================================*/
        constexpr bool is_poolable_size(size_t size) {
            return size <= get_block_size_for_class(NUM_SIZE_CLASSES - 1);
        }

        //=================================================================================
        // Alignment Utilities
        //=================================================================================

        /*=================================================================================
        Function Brief:
        Aligns an address to the specified alignment boundary.

        Parameters:
        address - The address to align.
        alignment - The alignment boundary (must be power of 2).

        Returns:
        The aligned address (>= original address).
        ==================================================================================*/
        constexpr size_t align_address(size_t address, size_t alignment) {
            return (address + alignment - 1) & ~(alignment - 1);
        }

        /*=================================================================================
        Function Brief:
        Aligns a pointer to the specified alignment boundary.

        Parameters:
        ptr - The pointer to align.
        alignment - The alignment boundary (must be power of 2).

        Returns:
        The aligned pointer (>= original pointer).
        ==================================================================================*/
        inline void* align_pointer(void* ptr, size_t alignment) {
            return reinterpret_cast<void*>(
                align_address(reinterpret_cast<size_t>(ptr), alignment)
                );
        }

    } // namespace Memory
} // namespace Engine
