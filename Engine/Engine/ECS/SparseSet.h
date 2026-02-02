//-----------------------------------------------------------------------------------
// file:    SparseSet.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the SparseSet data structure for efficient entity storage
//          in the ECS. Provides O(1) insertion, deletion, and lookup operations while
//          maintaining dense iteration. Useful for component arrays and entity sets.
//
//          Now uses the custom Memory Manager for all allocations. The underlying
//          storage uses Memory::Array instead of std::vector, eliminating STL
//          container usage and ensuring all memory flows through the pool allocators.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "../Memory/Memory.h"
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace Engine {
  namespace ECS {

    /* ============================================================ //
    // SparseSet - Efficient storage with O(1) operations          //
    // Uses Memory Manager for all allocations (no STL containers) //
    // ============================================================ */
    template<typename T = size_t>
    class SparseSet {
    private:
      Memory::Array<size_t> sparse;    // Maps key to dense array index
      Memory::Array<size_t> dense;     // Dense array of keys
      Memory::Array<T> data;           // Dense array of data (optional)

      static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

    public:
      /*=================================================================================
      Function Brief:
      Constructs an empty SparseSet with optional initial capacity.

      Parameters:
      reserve_size - Initial capacity for the sparse array (optional).

      Where to call it:
      When creating a new sparse set for component storage or entity tracking.
      ==================================================================================*/
      SparseSet(size_t reserve_size = 0);

      /*=================================================================================
      Function Brief:
      Inserts a key into the sparse set. If the key already exists, does nothing.

      Parameters:
      key - The key to insert.

      Where to call it:
      When adding a key to the set (e.g., an entity to a component array or system entity set).
      ==================================================================================*/
      void insert(size_t key);

      /*=================================================================================
      Function Brief:
      Inserts a key with associated data into the sparse set.

      Parameters:
      key - The key to insert.
      value - The data to associate with the key.

      Where to call it:
      When adding data associated with a key (e.g., a component to an entity in a component array).
      ==================================================================================*/
      void insert(size_t key, const T& value);

      /*=================================================================================
      Function Brief:
      Inserts a key with associated data into the sparse set (move version).

      Parameters:
      key - The key to insert.
      value - The data to move and associate with the key.

      Where to call it:
      When adding data associated with a key using move semantics for efficiency.
      ==================================================================================*/
      void insert(size_t key, T&& value);

      /*=================================================================================
      Function Brief:
      Constructs data in-place for a key.

      Parameters:
      key - The key to insert.
      args - Arguments forwarded to the constructor of T.

      Where to call it:
      When constructing component data directly without creating a temporary.
      ==================================================================================*/
      template<typename... Args>
      void emplace(size_t key, Args&&... args);

      /*=================================================================================
      Function Brief:
      Removes a key from the sparse set using swap-and-pop for O(1) removal.

      Parameters:
      key - The key to remove.

      Where to call it:
      When removing data from the set (e.g., a component from an entity or an entity from a system).
      ==================================================================================*/
      void erase(size_t key);

      /*=================================================================================
      Function Brief:
      Checks if the sparse set contains the specified key.

      Parameters:
      key - The key to check.

      Where to call it:
      When verifying if a key exists in the set (e.g., if an entity has a specific component).
      ==================================================================================*/
      bool contains(size_t key) const noexcept;

      /*=================================================================================
      Function Brief:
      Gets the data associated with a key. Returns nullptr if not found.

      Parameters:
      key - The key to get data for.

      Where to call it:
      When accessing data associated with a key (e.g., a component's data from an entity).
      ==================================================================================*/
      T* get(size_t key) noexcept;

      /*=================================================================================
      Function Brief:
      Gets the data associated with a key (const version).

      Parameters:
      key - The key to get data for.

      Where to call it:
      When accessing data associated with a key without modifying it (e.g., a component's data from an entity).
      ==================================================================================*/
      const T* get(size_t key) const noexcept;

      /*=================================================================================
      Function Brief:
      Gets the data at a specific index in the dense array.

      Parameters:
      index - The index in the dense array.

      Where to call it:
      When iterating over all components in dense order.
      ==================================================================================*/
      T& at(size_t index);

      /*=================================================================================
      Function Brief:
      Gets the data at a specific index in the dense array (const version).

      Parameters:
      index - The index in the dense array.

      Where to call it:
      When iterating over all components in dense order without modifying them.
      ==================================================================================*/
      const T& at(size_t index) const;

      /*=================================================================================
      Function Brief:
      Gets the key at a specific index in the dense array.

      Parameters:
      index - The index in the dense array.

      Where to call it:
      When iterating over all keys in the set (e.g., all entities).
      ==================================================================================*/
      size_t key_at(size_t index) const;

      /*=================================================================================
      Function Brief:
      Returns the number of entities in the sparse set.

      Parameters:
      None.

      Where to call it:
      When checking how many entities have a specific component.
      ==================================================================================*/
      size_t size() const noexcept;

      /*=================================================================================
      Function Brief:
      Checks if the sparse set is empty.

      Parameters:
      None.

      Where to call it:
      When checking if any entities have a specific component.
      ==================================================================================*/
      bool empty() const noexcept;

      /*=================================================================================
      Function Brief:
      Clears all entities and data from the sparse set.

      Parameters:
      None.

      Where to call it:
      When resetting a component array or entity set.
      ==================================================================================*/
      void clear();

      /*=================================================================================
      Function Brief:
      Reserves space in the sparse array to reduce reallocations.

      Parameters:
      capacity - The number of entity IDs to reserve space for.

      Where to call it:
      When you know the maximum entity ID in advance.
      ==================================================================================*/
      void reserve(size_t capacity);

      /*=================================================================================
      Function Brief:
      Inserts a range of keys into the sparse set.

      Parameters:
      begin - Iterator to the beginning of the range.
      end - Iterator to the end of the range.

      Where to call it:
      When bulk inserting multiple keys at once.
      ==================================================================================*/
      template<typename Iterator>
      void insert_range(Iterator begin, Iterator end);

      /*=================================================================================
      Function Brief:
      Removes all keys that satisfy a predicate.

      Parameters:
      predicate - Function that returns true for keys to remove.

      Where to call it:
      When conditionally removing multiple elements.
      ==================================================================================*/
      template<typename Predicate>
      void erase_if(Predicate predicate);

      /*=================================================================================
      Function Brief:
      Sorts the dense array using a custom comparator function.

      Parameters:
      compare - Comparator function that takes two keys and returns true if
                the first should come before the second.

      Where to call it:
      When you need to sort keys by a specific criteria (e.g., entities by layer priority).
      ==================================================================================*/
      template<typename Compare>
      void sort(Compare compare);

      /*=================================================================================
      Function Brief:
      Returns an iterator to the beginning of the dense key array.

      Parameters:
      None.

      Where to call it:
      When iterating over all keys in the set.
      ==================================================================================*/
      auto begin() { return dense.begin(); }

      /*=================================================================================
      Function Brief:
      Returns an iterator to the end of the dense key array.

      Parameters:
      None.

      Where to call it:
      When iterating over all keys in the set.
      ==================================================================================*/
      auto end() { return dense.end(); }

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the beginning of the dense key array.

      Parameters:
      None.

      Where to call it:
      When iterating over all keys in the set without modifying them.
      ==================================================================================*/
      auto begin() const { return dense.begin(); }

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the end of the dense key array.

      Parameters:
      None.

      Where to call it:
      When iterating over all keys in the set without modifying them.
      ==================================================================================*/
      auto end() const { return dense.end(); }

      /*=================================================================================
      Function Brief:
      Returns an iterator to the beginning of the data array.

      Parameters:
      None.

      Where to call it:
      When iterating over all component data in dense order.
      ==================================================================================*/
      auto data_begin() { return data.begin(); }

      /*=================================================================================
      Function Brief:
      Returns an iterator to the end of the data array.

      Parameters:
      None.

      Where to call it:
      When iterating over all component data in dense order.
      ==================================================================================*/
      auto data_end() { return data.end(); }

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the beginning of the data array.

      Parameters:
      None.

      Where to call it:
      When iterating over all component data without modifying them.
      ==================================================================================*/
      auto data_begin() const { return data.begin(); }

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the end of the data array.

      Parameters:
      None.

      Where to call it:
      When iterating over all component data without modifying them.
      ==================================================================================*/
      auto data_end() const { return data.end(); }

      /*=================================================================================
      Function Brief:
      Returns a range view of all keys in the set.

      Parameters:
      None.

      Where to call it:
      When iterating over keys using range-based for loops.
      ==================================================================================*/
      const Memory::Array<size_t>& keys() const noexcept { return dense; }

      /*=================================================================================
      Function Brief:
      Returns a range view of all values in the set.

      Parameters:
      None.

      Where to call it:
      When iterating over values using range-based for loops.
      ==================================================================================*/
      const Memory::Array<T>& values() const noexcept requires (!std::is_same_v<T, size_t>) { return data; }
      Memory::Array<T>& values() noexcept requires (!std::is_same_v<T, size_t>) { return data; }
    };

    // ============================================================================
    // SparseSet<void> - Key-only specialization (no data storage)
    // Uses Memory Manager for all allocations
    // ============================================================================
    template<>
    class SparseSet<void> {
    private:
      Memory::Array<size_t> sparse;
      Memory::Array<size_t> dense;
      static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

    public:
      SparseSet(size_t reserve_size = 0);
      void insert(size_t key);
      void erase(size_t key);
      bool contains(size_t key) const noexcept;
      size_t key_at(size_t index) const;
      size_t size() const noexcept;
      bool empty() const noexcept;
      void clear();
      void reserve(size_t capacity);

      template<typename Iterator>
      void insert_range(Iterator begin, Iterator end);

      template<typename Predicate>
      void erase_if(Predicate predicate);

      template<typename Compare>
      void sort(Compare compare);

      auto begin() { return dense.begin(); }
      auto end() { return dense.end(); }
      auto begin() const { return dense.begin(); }
      auto end() const { return dense.end(); }
      const Memory::Array<size_t>& keys() const noexcept { return dense; }
    };

    // ============================================================================
    // Template Implementation
    // ============================================================================

    template<typename T>
    SparseSet<T>::SparseSet(size_t reserve_size) {
      if (reserve_size > 0) {
        sparse.resize(reserve_size, NULL_INDEX);
      }
    }

    template<typename T>
    void SparseSet<T>::insert(size_t key) {
      if (contains(key)) {
        return; // Already exists
      }

      // Expand sparse array if needed
      if (key >= sparse.size()) {
        sparse.resize(key + 1, NULL_INDEX);
      }

      // Add to dense array and update sparse mapping
      sparse[key] = dense.size();
      dense.push_back(key);

      // Add default-constructed data
      if constexpr (!std::is_same_v<T, size_t>) {
        data.emplace_back();
      }
    }

    template<typename T>
    void SparseSet<T>::insert(size_t key, const T& value) {
      if (contains(key)) {
        // Update existing value
        size_t index = sparse[key];
        if constexpr (!std::is_same_v<T, size_t>) {
          data[index] = value;
        }
        return;
      }

      // Expand sparse array if needed
      if (key >= sparse.size()) {
        sparse.resize(key + 1, NULL_INDEX);
      }

      // Add to dense array and update sparse mapping
      sparse[key] = dense.size();
      dense.push_back(key);

      // Add data
      if constexpr (!std::is_same_v<T, size_t>) {
        data.push_back(value);
      }
    }

    template<typename T>
    void SparseSet<T>::insert(size_t key, T&& value) {
      if (contains(key)) {
        // Update existing value
        size_t index = sparse[key];
        if constexpr (!std::is_same_v<T, size_t>) {
          data[index] = std::move(value);
        }
        return;
      }

      // Expand sparse array if needed
      if (key >= sparse.size()) {
        sparse.resize(key + 1, NULL_INDEX);
      }

      // Add to dense array and update sparse mapping
      sparse[key] = dense.size();
      dense.push_back(key);

      // Add data
      if constexpr (!std::is_same_v<T, size_t>) {
        data.push_back(std::move(value));
      }
    }

    template<typename T>
    template<typename... Args>
    void SparseSet<T>::emplace(size_t key, Args&&... args) {
      if (contains(key)) {
        // Update existing value
        size_t index = sparse[key];
        if constexpr (!std::is_same_v<T, size_t>) {
          data[index] = T(std::forward<Args>(args)...);
        }
        return;
      }

      // Expand sparse array if needed
      if (key >= sparse.size()) {
        sparse.resize(key + 1, NULL_INDEX);
      }

      // Add to dense array and update sparse mapping
      sparse[key] = dense.size();
      dense.push_back(key);

      // Emplace data
      if constexpr (!std::is_same_v<T, size_t>) {
        data.emplace_back(std::forward<Args>(args)...);
      }
    }

    template<typename T>
    void SparseSet<T>::erase(size_t key) {
      if (!contains(key)) {
        return; // Key not in set
      }

      // Swap-and-pop removal for O(1) deletion
      size_t index = sparse[key];
      size_t last = dense.back();

      // Swap with last element
      dense[index] = last;
      sparse[last] = index;

      if constexpr (!std::is_same_v<T, size_t>) {
        data[index] = std::move(data.back());
        data.pop_back();
      }

      // Remove last element
      dense.pop_back();
      sparse[key] = NULL_INDEX;
    }

    template<typename T>
    bool SparseSet<T>::contains(size_t key) const noexcept {
      return key < sparse.size() && sparse[key] != NULL_INDEX;
    }

    template<typename T>
    T* SparseSet<T>::get(size_t key) noexcept {
      if (!contains(key)) {
        return nullptr;
      }
      if constexpr (std::is_same_v<T, size_t>) {
        return nullptr; // No data storage when T == size_t
      }
      else {
        size_t index = sparse[key];
        return &data[index];
      }
    }

    template<typename T>
    const T* SparseSet<T>::get(size_t key) const noexcept {
      if (!contains(key)) {
        return nullptr;
      }
      if constexpr (std::is_same_v<T, size_t>) {
        return nullptr; // No data storage when T == size_t
      }
      else {
        size_t index = sparse[key];
        return &data[index];
      }
    }

    template<typename T>
    T& SparseSet<T>::at(size_t index) {
      if constexpr (std::is_same_v<T, size_t>) {
        throw std::runtime_error("Cannot access data when T == size_t");
      }
      else {
#ifdef _DEBUG
        if (index >= data.size()) {
          throw std::out_of_range("Index out of range");
        }
#endif
        return data[index];
      }
    }

    template<typename T>
    const T& SparseSet<T>::at(size_t index) const {
      if constexpr (std::is_same_v<T, size_t>) {
        throw std::runtime_error("Cannot access data when T == size_t");
      }
      else {
#ifdef _DEBUG
        if (index >= data.size()) {
          throw std::out_of_range("Index out of range");
        }
#endif
        return data[index];
      }
    }

    template<typename T>
    size_t SparseSet<T>::key_at(size_t index) const {
#ifdef _DEBUG
      if (index >= dense.size()) {
        throw std::out_of_range("Index out of range");
      }
#endif
      return dense[index];
    }

    template<typename T>
    size_t SparseSet<T>::size() const noexcept {
      return dense.size();
    }

    template<typename T>
    bool SparseSet<T>::empty() const noexcept {
      return dense.empty();
    }

    template<typename T>
    void SparseSet<T>::clear() {
      sparse.clear();
      dense.clear();
      if constexpr (!std::is_same_v<T, size_t>) {
        data.clear();
      }
    }

    template<typename T>
    void SparseSet<T>::reserve(size_t capacity) {
      if (capacity > sparse.size()) {
        sparse.resize(capacity, NULL_INDEX);
      }
      dense.reserve(capacity);
      if constexpr (!std::is_same_v<T, size_t>) {
        data.reserve(capacity);
      }
    }

    template<typename T>
    template<typename Iterator>
    void SparseSet<T>::insert_range(Iterator begin, Iterator end) {
      for (auto it = begin; it != end; ++it) {
        insert(*it);
      }
    }

    template<typename T>
    template<typename Predicate>
    void SparseSet<T>::erase_if(Predicate predicate) {
      // Iterate backwards to avoid invalidation issues
      for (size_t i = dense.size(); i-- > 0; ) {
        if (predicate(dense[i])) {
          erase(dense[i]);
        }
      }
    }

    template<typename T>
    template<typename Compare>
    void SparseSet<T>::sort(Compare compare) {
      // Create indices array to track the new order using Memory::Array
      Memory::Array<size_t> indices(dense.size());
      for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
      }

      // Simple insertion sort (efficient for small arrays, stable)
      // For large arrays, consider implementing quicksort
      for (size_t i = 1; i < indices.size(); ++i) {
        size_t key = indices[i];
        size_t j = i;
        while (j > 0 && compare(dense[key], dense[indices[j - 1]])) {
          indices[j] = indices[j - 1];
          --j;
        }
        indices[j] = key;
      }

      // Apply permutation in-place using cycles
      for (size_t i = 0; i < indices.size(); ++i) {
        while (indices[i] != i) {
          size_t target = indices[i];
          // Swap dense[i] and dense[target]
          size_t temp_key = dense[i];
          dense[i] = dense[target];
          dense[target] = temp_key;

          if constexpr (!std::is_same_v<T, size_t>) {
            T temp_data = std::move(data[i]);
            data[i] = std::move(data[target]);
            data[target] = std::move(temp_data);
          }

          size_t temp_idx = indices[i];
          indices[i] = indices[target];
          indices[target] = temp_idx;
        }
      }

      // Update sparse array to point to new indices
      for (size_t i = 0; i < dense.size(); ++i) {
        sparse[dense[i]] = i;
      }
    }

    // ============================================================================
    // SparseSet<void> Implementation
    // ============================================================================

    inline SparseSet<void>::SparseSet(size_t reserve_size) {
      if (reserve_size > 0) {
        sparse.resize(reserve_size, NULL_INDEX);
      }
    }

    inline void SparseSet<void>::insert(size_t key) {
      if (contains(key)) {
        return;
      }

      if (key >= sparse.size()) {
        sparse.resize(key + 1, NULL_INDEX);
      }

      sparse[key] = dense.size();
      dense.push_back(key);
    }

    inline void SparseSet<void>::erase(size_t key) {
      if (!contains(key)) {
        return;
      }

      size_t index = sparse[key];
      size_t last = dense.back();

      dense[index] = last;
      sparse[last] = index;

      dense.pop_back();
      sparse[key] = NULL_INDEX;
    }

    inline bool SparseSet<void>::contains(size_t key) const noexcept {
      return key < sparse.size() && sparse[key] != NULL_INDEX;
    }

    inline size_t SparseSet<void>::key_at(size_t index) const {
#ifdef _DEBUG
      if (index >= dense.size()) {
        throw std::out_of_range("Index out of range");
      }
#endif
      return dense[index];
    }

    inline size_t SparseSet<void>::size() const noexcept {
      return dense.size();
    }

    inline bool SparseSet<void>::empty() const noexcept {
      return dense.empty();
    }

    inline void SparseSet<void>::clear() {
      sparse.clear();
      dense.clear();
    }

    inline void SparseSet<void>::reserve(size_t capacity) {
      if (capacity > sparse.size()) {
        sparse.resize(capacity, NULL_INDEX);
      }
      dense.reserve(capacity);
    }

    template<typename Iterator>
    void SparseSet<void>::insert_range(Iterator begin, Iterator end) {
      for (auto it = begin; it != end; ++it) {
        insert(*it);
      }
    }

    template<typename Predicate>
    void SparseSet<void>::erase_if(Predicate predicate) {
      for (size_t i = dense.size(); i-- > 0; ) {
        if (predicate(dense[i])) {
          erase(dense[i]);
        }
      }
    }

    template<typename Compare>
    void SparseSet<void>::sort(Compare compare) {
      Memory::Array<size_t> indices(dense.size());
      for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
      }

      // Simple insertion sort
      for (size_t i = 1; i < indices.size(); ++i) {
        size_t key = indices[i];
        size_t j = i;
        while (j > 0 && compare(dense[key], dense[indices[j - 1]])) {
          indices[j] = indices[j - 1];
          --j;
        }
        indices[j] = key;
      }

      for (size_t i = 0; i < indices.size(); ++i) {
        while (indices[i] != i) {
          size_t target = indices[i];
          size_t temp = dense[i];
          dense[i] = dense[target];
          dense[target] = temp;

          size_t temp_idx = indices[i];
          indices[i] = indices[target];
          indices[target] = temp_idx;
        }
      }

      for (size_t i = 0; i < dense.size(); ++i) {
        sparse[dense[i]] = i;
      }
    }

  } // namespace ECS
} // namespace Engine
