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
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace Engine {
  namespace ECS {

    /* ============================================================ //
    // SparseSet - Efficient storage with O(1) operations          //
    // ============================================================ */
    template<typename T = size_t>
    class SparseSet {
    private:
      std::vector<size_t> sparse;    // Maps key to dense array index
      std::vector<size_t> dense;     // Dense array of keys
      std::vector<T> data;           // Dense array of data (optional)

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
      bool contains(size_t key) const;

      /*=================================================================================
      Function Brief:
      Gets the data associated with a key. Returns nullptr if not found.

      Parameters:
      key - The key to get data for.

      Where to call it:
      When accessing data associated with a key (e.g., a component's data from an entity).
      ==================================================================================*/
      T* get(size_t key);

      /*=================================================================================
      Function Brief:
      Gets the data associated with a key (const version).

      Parameters:
      key - The key to get data for.

      Where to call it:
      When accessing data associated with a key without modifying it (e.g., a component's data from an entity).
      ==================================================================================*/
      const T* get(size_t key) const;

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
      size_t size() const;

      /*=================================================================================
      Function Brief:
      Checks if the sparse set is empty.

      Parameters:
      None.

      Where to call it:
      When checking if any entities have a specific component.
      ==================================================================================*/
      bool empty() const;

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
    bool SparseSet<T>::contains(size_t key) const {
      return key < sparse.size() && sparse[key] != NULL_INDEX;
    }

    template<typename T>
    T* SparseSet<T>::get(size_t key) {
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
    const T* SparseSet<T>::get(size_t key) const {
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
        if (index >= data.size()) {
          throw std::out_of_range("Index out of range");
        }
        return data[index];
      }
    }

    template<typename T>
    const T& SparseSet<T>::at(size_t index) const {
      if constexpr (std::is_same_v<T, size_t>) {
        throw std::runtime_error("Cannot access data when T == size_t");
      }
      else {
        if (index >= data.size()) {
          throw std::out_of_range("Index out of range");
        }
        return data[index];
      }
    }

    template<typename T>
    size_t SparseSet<T>::key_at(size_t index) const {
      if (index >= dense.size()) {
        throw std::out_of_range("Index out of range");
      }
      return dense[index];
    }

    template<typename T>
    size_t SparseSet<T>::size() const {
      return dense.size();
    }

    template<typename T>
    bool SparseSet<T>::empty() const {
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
      sparse.resize(capacity, NULL_INDEX);
      dense.reserve(capacity);
      if constexpr (!std::is_same_v<T, size_t>) {
        data.reserve(capacity);
      }
    }

    template<typename T>
    template<typename Compare>
    void SparseSet<T>::sort(Compare compare) {
      // Create indices array to track the new order
      std::vector<size_t> indices(dense.size());
      for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
      }

      // Sort indices based on the comparator applied to dense array
      std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return compare(dense[a], dense[b]);
        });

      // Create temporary arrays for the sorted data
      std::vector<size_t> sorted_dense;
      sorted_dense.reserve(dense.size());

      std::vector<T> sorted_data;
      if constexpr (!std::is_same_v<T, size_t>) {
        sorted_data.reserve(data.size());
      }

      // Copy elements in sorted order
      for (size_t i = 0; i < indices.size(); ++i) {
        size_t old_index = indices[i];
        sorted_dense.push_back(dense[old_index]);

        if constexpr (!std::is_same_v<T, size_t>) {
          sorted_data.push_back(data[old_index]);
        }
      }

      // Replace with sorted arrays
      dense = std::move(sorted_dense);
      if constexpr (!std::is_same_v<T, size_t>) {
        data = std::move(sorted_data);
      }

      // Update sparse array to point to new indices
      for (size_t i = 0; i < dense.size(); ++i) {
        sparse[dense[i]] = i;
      }
    }

  } // namespace ECS
} // namespace Engine
