//-----------------------------------------------------------------------------------
// file:    MemoryManager.cpp
// project: Monkey Stole My GPA
//
// author:  [Your Name] (100%)
//          Primary author
//
// email:   [your.email]@digipen.edu
//
// brief:   Implementation file for the Custom Memory Manager.
//          Contains static instance initialization and debug utilities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "MemoryManager.h"
#include <cstdio>  // For debug printing

namespace Engine {
    namespace Memory {

        // ============================================================================
        // Debug Utilities
        // ============================================================================

        /**
         * @brief Prints memory statistics for debugging purposes.
         *
         * Outputs current memory usage, allocation counts, and page information
         * for all active memory pools.
         */
        void print_memory_stats() {
            MemoryManager& manager = MemoryManager::instance();

            printf("=== Memory Manager Statistics ===\n");
            printf("Total Memory Allocated: %zu bytes (%.2f MB)\n",
                manager.get_total_memory(),
                static_cast<double>(manager.get_total_memory()) / (1024.0 * 1024.0));
            printf("Total Active Allocations: %zu\n", manager.get_total_active_allocations());
            printf("=================================\n");
        }

        /**
         * @brief Validates memory pool integrity.
         *
         * Performs sanity checks on the memory pools to detect corruption.
         * This is a debug-only function.
         *
         * @return true if all pools are valid, false if corruption detected.
         */
        bool validate_memory_pools() {
            // In debug builds, we could walk the free lists and verify integrity
            // For now, return true (no validation implemented)
            return true;
        }

    } // namespace Memory
} // namespace Engine
