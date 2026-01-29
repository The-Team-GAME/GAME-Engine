# Custom Memory Manager Documentation

## Overview

This document describes the Custom Memory Manager implementation for the Monkey Stole My GPA game engine. The memory manager provides efficient, pre-allocated memory for game objects (entities) and components, eliminating runtime OS allocations during gameplay.

## File Structure

```
Engine/Engine/
├── Memory/
│   ├── MemoryManager.h      # Main memory manager with Pool Allocator
│   └── MemoryManager.cpp    # Implementation and debug utilities
└── ECS/
    ├── ManagedSparseSet.h       # STL-free sparse set + ManagedArray
    ├── ManagedContainers.h      # ManagedHashMap, ManagedHashSet, ManagedUniquePtr
    ├── ManagedComponentArray.h  # Component storage using pool allocator
    ├── ManagedWorldData.h       # WorldContainers using all managed types
    ├── ManagedWorldData.cpp     # Implementation
    └── ECS.h                    # Updated to include memory manager
```

## STL Replacements

| STL Container             | Managed Replacement   | File                |
| ------------------------- | --------------------- | ------------------- |
| `std::vector<T>`          | `ManagedArray<T>`     | ManagedSparseSet.h  |
| `std::unordered_map<K,V>` | `ManagedHashMap<K,V>` | ManagedContainers.h |
| `std::unordered_set<K>`   | `ManagedHashSet<K>`   | ManagedContainers.h |
| `std::unique_ptr<T>`      | `ManagedUniquePtr<T>` | ManagedContainers.h |
| `SparseSet<T>`            | `ManagedSparseSet<T>` | ManagedSparseSet.h  |

## Key Features

### 1. Pool Allocator with Free List

The Pool Allocator provides O(1) allocation and deallocation by maintaining a free list of available memory blocks.

**How it works:**

- Memory is pre-allocated in large "pages" (default 4MB)
- Each page is divided into fixed-size "blocks"
- Free blocks are linked together in a free list (the link is embedded in the block itself)
- Allocation: Pop from free list head - O(1)
- Deallocation: Push to free list head - O(1)

```cpp
// Free list node is embedded in free blocks (no extra memory overhead)
struct FreeListNode {
  FreeListNode* next;
};
```

### 2. Memory Pages

Memory Pages are large contiguous chunks of memory allocated at load time.

```cpp
struct MemoryPage {
  MemoryPage* next_page;  // Linked list of pages
  u8* memory;             // Raw memory
  size_t size;            // Page size
  size_t used;            // Debug: bytes used
};
```

**Dynamic Extension:** When all pages are full and the free list is empty, a new page is automatically allocated.

### 3. Placement New

Objects are constructed in pre-allocated memory using C++ placement new:

```cpp
template<typename T, typename... Args>
T* PoolAllocator::construct(Args&&... args) {
  void* memory = allocate();  // Get memory from free list
  if (!memory) return nullptr;

  // Use placement new to construct object in pre-allocated memory
  return new (memory) T(std::forward<Args>(args)...);
}
```

### 4. No STL Containers in Allocator

The memory manager and ManagedSparseSet use raw memory operations:

```cpp
// ManagedArray - STL-free dynamic array
template<typename T>
class ManagedArray {
private:
  T* data_;        // Raw pointer to data
  size_t size_;    // Current size
  size_t capacity_;// Allocated capacity
  // Uses malloc/free, placement new for elements
};
```

## Usage Guide

### Basic Usage

```cpp
#include "ECS/ECS.h"
using namespace Engine::Memory;
using namespace Engine::ECS;

// ===== Option 1: Use POOL_NEW/POOL_DELETE macros =====
MyComponent* comp = POOL_NEW(MyComponent, arg1, arg2);
// ... use component ...
POOL_DELETE(MyComponent, comp);

// ===== Option 2: Use MemoryManager directly =====
MemoryManager& mm = MemoryManager::instance();
auto* obj = mm.allocate<MyComponent>(arg1, arg2);
// ... use object ...
mm.deallocate(obj);

// ===== Option 3: Use TypedPoolAllocator for a specific type =====
TypedPoolAllocator<MyComponent> pool;
auto* comp = pool.construct(arg1, arg2);
// ... use component ...
pool.destruct(comp);
```

### Integration with ECS

#### Using ManagedComponentArray

Replace standard ComponentArray with ManagedComponentArray for pool-allocated components:

```cpp
// Instead of:
// ComponentArray<Transform> transforms;

// Use:
ManagedComponentArray<Transform> transforms;

// Add component (uses pool allocation + placement new)
transforms.add_component(entityId, Transform{});

// Get component
Transform* t = transforms.get_component(entityId);

// Remove component (calls destructor, returns memory to pool)
transforms.remove_entity(entityId);

// Pre-allocate for expected count (during level load)
transforms.reserve(1000);  // Expect ~1000 transforms
```

#### Memory Statistics

```cpp
// Get statistics for a component array
ManagedComponentArray<Transform>& transforms = ...;
size_t memUsed = transforms.get_memory_usage();
size_t allocCount = transforms.get_allocation_count();

// Global statistics
MemoryManager& mm = MemoryManager::instance();
size_t totalMem = mm.get_total_memory();
size_t activeAllocs = mm.get_total_active_allocations();

// Debug print
Engine::Memory::print_memory_stats();
```

### Level Load/Unload

```cpp
void LoadLevel() {
  // Pre-allocate memory based on expected entity count
  MemoryManager& mm = MemoryManager::instance();

  // Get pools for common components and reserve pages
  mm.get_pool<Transform>().reserve_pages(2);   // 2 pages for transforms
  mm.get_pool<Sprite>().reserve_pages(3);      // 3 pages for sprites
  mm.get_pool<Collider>().reserve_pages(1);    // 1 page for colliders

  // Now load level - no OS allocations during gameplay!
}

void UnloadLevel() {
  // Option 1: Reset specific component arrays
  transformArray.reset_memory();

  // Option 2: Reset all pools (faster for level unload)
  MemoryManager::instance().reset_all();
}
```

## Technical Details

### Memory Layout

```
Page Structure:
┌─────────────────────────────────────────────────────────┐
│ MemoryPage Header (24 bytes)                            │
├─────────────────────────────────────────────────────────┤
│ Block 0  │ Block 1  │ Block 2  │ ... │ Block N         │
└─────────────────────────────────────────────────────────┘

Free Block (when not in use):
┌─────────────────────────────────────────────────────────┐
│ FreeListNode* next │ [unused space]                     │
└─────────────────────────────────────────────────────────┘

Allocated Block:
┌─────────────────────────────────────────────────────────┐
│ Component Data (constructed via placement new)          │
└─────────────────────────────────────────────────────────┘
```

### Time Complexity

| Operation        | Complexity | Notes                    |
| ---------------- | ---------- | ------------------------ |
| Allocate         | O(1)       | Pop from free list       |
| Deallocate       | O(1)       | Push to free list        |
| Add Component    | O(1)       | Allocate + placement new |
| Remove Component | O(1)       | Destructor + deallocate  |
| Get Component    | O(1)       | Sparse set lookup        |
| Page Allocation  | O(n)       | Only when out of memory  |

### Configuration

```cpp
// In MemoryManager.h
inline constexpr size_t DEFAULT_PAGE_SIZE = 4 * 1024 * 1024;  // 4MB
inline constexpr size_t MIN_BLOCK_SIZE = sizeof(void*);       // 8 bytes
inline constexpr size_t DEFAULT_ALIGNMENT = 16;               // Cache line friendly
```

## Compliance with Rubric

| Requirement                       | Implementation                                            |
| --------------------------------- | --------------------------------------------------------- |
| Pre-allocated memory at load time | ✅ MemoryPage::create() allocates pages at initialization |
| Placement new for construction    | ✅ PoolAllocator::construct() uses placement new          |
| Free linked list                  | ✅ FreeListNode embedded in free blocks                   |
| Memory pages for blocks           | ✅ MemoryPage struct with linked page list                |
| No STL containers in allocator    | ✅ Uses raw arrays and malloc/free                        |
| Dynamic page extension            | ✅ PoolAllocator::add_page() when out of memory           |
| Memory not freed on deallocation  | ✅ Deallocate just pushes to free list                    |
| Used for game objects/components  | ✅ ManagedComponentArray integration                      |

## Example Integration

Here's how to integrate the memory manager with your existing ECS:

```cpp
// In your game initialization (e.g., Core.cpp)
#include "ECS/ECS.h"

void Engine::Core::Init() {
  // Pre-allocate memory pools for expected game objects
  auto& mm = Engine::Memory::MemoryManager::instance();

  // Calculate expected counts from level data
  size_t expectedEntities = 500;

  // Reserve memory for common components
  mm.get_pool<Transform>(1024 * 1024).reserve_pages(1);  // 1MB page
  mm.get_pool<Sprite>(2 * 1024 * 1024).reserve_pages(1); // 2MB page

  Engine::Memory::print_memory_stats();
}

void Engine::Core::Shutdown() {
  // Memory is automatically cleaned up by destructors
  // But you can explicitly reset if needed:
  Engine::Memory::MemoryManager::instance().reset_all();
}
```

## Debugging

Enable memory debugging by calling:

```cpp
// Print current memory statistics
Engine::Memory::print_memory_stats();

// Example output:
// === Memory Manager Statistics ===
// Total Memory Allocated: 12582912 bytes (12.00 MB)
// Total Active Allocations: 1247
// =================================

// Validate pool integrity (debug builds)
bool valid = Engine::Memory::validate_memory_pools();
```

## Best Practices

1. **Pre-allocate at level load**: Call `reserve_pages()` based on expected entity counts
2. **Use ManagedComponentArray**: For frequently created/destroyed components
3. **Reset on level unload**: Call `reset_all()` or `reset_memory()` instead of individual deletes
4. **Monitor memory usage**: Use `print_memory_stats()` during development
5. **Tune page sizes**: Adjust based on your game's memory patterns

## Limitations

- Components must be trivially relocatable (no self-referential pointers)
- Page memory is never returned to OS (only recycled via free list)
- Not thread-safe (add mutex if needed for multi-threaded access)
