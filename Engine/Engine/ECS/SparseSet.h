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

namespace Engine {
  namespace ECS {

    /* ============================================================ //
    // SparseSet - Efficient entity storage with O(1) operations   //
    // ============================================================ */
    template<typename T = EntityID>
    class SparseSet {
    private:
      std::vector<EntityID> sparse;  // Maps entity ID to dense array index
      std::vector<EntityID> dense;   // Dense array of entity IDs
      std::vector<T> data;           // Dense array of component data (optional)

      static constexpr EntityID NULL_INDEX = static_cast<EntityID>(-1);

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
      Inserts an entity into the sparse set. If the entity already exists, does nothing.

      Parameters:
      entity - The EntityID to insert.

      Where to call it:
      When adding an entity to a component array or system entity set.
      ==================================================================================*/
      void insert(EntityID entity);

      /*=================================================================================
      Function Brief:
      Inserts an entity with associated data into the sparse set.

      Parameters:
      entity - The EntityID to insert.
      value - The data to associate with the entity.

      Where to call it:
      When adding a component to an entity in a component array.
      ==================================================================================*/
      void insert(EntityID entity, const T& value);

      /*=================================================================================
      Function Brief:
      Removes an entity from the sparse set using swap-and-pop for O(1) removal.

      Parameters:
      entity - The EntityID to remove.

      Where to call it:
      When removing a component from an entity or removing an entity from a system.
      ==================================================================================*/
      void erase(EntityID entity);

      /*=================================================================================
      Function Brief:
      Checks if the sparse set contains the specified entity.

      Parameters:
      entity - The EntityID to check.

      Where to call it:
      When verifying if an entity has a specific component.
      ==================================================================================*/
      bool contains(EntityID entity) const;

      /*=================================================================================
      Function Brief:
      Gets the data associated with an entity. Returns nullptr if not found.

      Parameters:
      entity - The EntityID to get data for.

      Where to call it:
      When accessing a component's data from an entity.
      ==================================================================================*/
      T* get(EntityID entity);

      /*=================================================================================
      Function Brief:
      Gets the data associated with an entity (const version).

      Parameters:
      entity - The EntityID to get data for.

      Where to call it:
      When accessing a component's data from an entity without modifying it.
      ==================================================================================*/
      const T* get(EntityID entity) const;

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
      Gets the entity ID at a specific index in the dense array.

      Parameters:
      index - The index in the dense array.

      Where to call it:
      When iterating over all entities in the set.
      ==================================================================================*/
      EntityID entity_at(size_t index) const;

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
      Returns an iterator to the beginning of the dense entity array.

      Parameters:
      None.

      Where to call it:
      When iterating over all entities in the set.
      ==================================================================================*/
      auto begin() { return dense.begin(); }

      /*=================================================================================
      Function Brief:
      Returns an iterator to the end of the dense entity array.

      Parameters:
      None.

      Where to call it:
      When iterating over all entities in the set.
      ==================================================================================*/
      auto end() { return dense.end(); }

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the beginning of the dense entity array.

      Parameters:
      None.

      Where to call it:
      When iterating over all entities in the set without modifying them.
      ==================================================================================*/
      auto begin() const { return dense.begin(); }

      /*=================================================================================
      Function Brief:
      Returns a const iterator to the end of the dense entity array.

      Parameters:
      None.

      Where to call it:
      When iterating over all entities in the set without modifying them.
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
    void SparseSet<T>::insert(EntityID entity) {
      if (contains(entity)) {
        return; // Already exists
      }

      // Expand sparse array if needed
      if (entity >= sparse.size()) {
        sparse.resize(entity + 1, NULL_INDEX);
      }

      // Add to dense array and update sparse mapping
      sparse[entity] = static_cast<EntityID>(dense.size());
      dense.push_back(entity);

      // Add default-constructed data
      if constexpr (!std::is_same_v<T, EntityID>) {
        data.emplace_back();
      }
    }

    template<typename T>
    void SparseSet<T>::insert(EntityID entity, const T& value) {
      if (contains(entity)) {
        // Update existing value
        EntityID index = sparse[entity];
        if constexpr (!std::is_same_v<T, EntityID>) {
          data[index] = value;
        }
        return;
      }

      // Expand sparse array if needed
      if (entity >= sparse.size()) {
        sparse.resize(entity + 1, NULL_INDEX);
      }

      // Add to dense array and update sparse mapping
      sparse[entity] = static_cast<EntityID>(dense.size());
      dense.push_back(entity);

      // Add data
      if constexpr (!std::is_same_v<T, EntityID>) {
        data.push_back(value);
      }
    }

    template<typename T>
    void SparseSet<T>::erase(EntityID entity) {
      if (!contains(entity)) {
        return; // Entity not in set
      }

      // Swap-and-pop removal for O(1) deletion
      EntityID index = sparse[entity];
      EntityID last = dense.back();

      // Swap with last element
      dense[index] = last;
      sparse[last] = index;

      if constexpr (!std::is_same_v<T, EntityID>) {
        data[index] = std::move(data.back());
        data.pop_back();
      }

      // Remove last element
      dense.pop_back();
      sparse[entity] = NULL_INDEX;
    }

    template<typename T>
    bool SparseSet<T>::contains(EntityID entity) const {
      return entity < sparse.size() && sparse[entity] != NULL_INDEX;
    }

    template<typename T>
    T* SparseSet<T>::get(EntityID entity) {
      if (!contains(entity)) {
        return nullptr;
      }
      if constexpr (std::is_same_v<T, EntityID>) {
        return nullptr; // No data storage for EntityID specialization
      }
      else {
        EntityID index = sparse[entity];
        return &data[index];
      }
    }

    template<typename T>
    const T* SparseSet<T>::get(EntityID entity) const {
      if (!contains(entity)) {
        return nullptr;
      }
      if constexpr (std::is_same_v<T, EntityID>) {
        return nullptr; // No data storage for EntityID specialization
      }
      else {
        EntityID index = sparse[entity];
        return &data[index];
      }
    }

    template<typename T>
    T& SparseSet<T>::at(size_t index) {
      if constexpr (std::is_same_v<T, EntityID>) {
        throw std::runtime_error("Cannot access data in EntityID specialization");
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
      if constexpr (std::is_same_v<T, EntityID>) {
        throw std::runtime_error("Cannot access data in EntityID specialization");
      }
      else {
        if (index >= data.size()) {
          throw std::out_of_range("Index out of range");
        }
        return data[index];
      }
    }

    template<typename T>
    EntityID SparseSet<T>::entity_at(size_t index) const {
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
      if constexpr (!std::is_same_v<T, EntityID>) {
        data.clear();
      }
    }

    template<typename T>
    void SparseSet<T>::reserve(size_t capacity) {
      sparse.resize(capacity, NULL_INDEX);
      dense.reserve(capacity);
      if constexpr (!std::is_same_v<T, EntityID>) {
        data.reserve(capacity);
      }
    }

  } // namespace ECS
} // namespace Engine
