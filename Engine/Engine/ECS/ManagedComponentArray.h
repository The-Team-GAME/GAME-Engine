//-----------------------------------------------------------------------------------
// file:    ManagedComponentArray.h
// project: Monkey Stole My GPA
//
// author:  [Your Name] (100%)
//          Primary author
//
// email:   [your.email]@digipen.edu
//
// brief:   Component Array implementation using the Custom Memory Manager.
//          This provides an alternative to ComponentArray that uses pool-allocated
//          memory instead of STL containers, meeting the custom memory manager rubric.
//
//          Key Features:
//          - Uses PoolAllocator for component storage
//          - Uses ManagedSparseSet for entity-to-component mapping
//          - Placement new for component construction
//          - O(1) add, remove, and lookup operations
//          - No STL containers for core storage
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "ManagedSparseSet.h"
#include "ComponentManager.h"
#include "../Memory/MemoryManager.h"
#include "../Reflection/ReflectionLibrary.h"
#include <memory>
#include <string>

namespace Engine {
  namespace ECS {

    // ============================================================================
    // IManagedComponentArray - Interface for managed component storage
    // ============================================================================

    /**
     * @brief Interface for type-erased managed component array access.
     *
     * Same interface as IComponentArray but for managed storage.
     */
    class IManagedComponentArray {
    public:
      virtual ~IManagedComponentArray() = default;
      virtual void remove_entity(EntityID entity) = 0;
      virtual void clone_component(EntityID source, EntityID destination) = 0;
      virtual ComponentTypeID get_type_id() const = 0;
      virtual bool is_reflectable() const = 0;
      virtual std::string get_type_name() const = 0;
      virtual ReflectionLibrary::GenericValue to_generic_value(EntityID entity) const = 0;
      virtual bool add_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) = 0;
      virtual bool set_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) = 0;
      virtual void add_default_component(EntityID entity) = 0;
      virtual ManagedUniquePtr<IManagedComponentArray> create_empty() const = 0;

      // Memory manager specific methods
      virtual size_t get_memory_usage() const = 0;
      virtual size_t get_allocation_count() const = 0;
      virtual void reset_memory() = 0;
    };

    // ============================================================================
    // ManagedComponentArray - Type-safe component storage with custom memory
    // ============================================================================

    /**
     * @brief Type-safe component storage using the Custom Memory Manager.
     *
     * This class stores components of type T using:
     * - A TypedPoolAllocator<T> for actual component memory
     * - A ManagedSparseSet<T*> for entity-to-component mapping
     *
     * Components are constructed using placement new in pre-allocated memory.
     * When removed, the destructor is called and memory is returned to the pool.
     *
     * @tparam T The component type to store.
     */
    template<typename T>
    class ManagedComponentArray : public IManagedComponentArray {
    private:
      /// Pool allocator for component memory
      Memory::TypedPoolAllocator<T> pool;

      /// Sparse set mapping entity IDs to component pointers
      ManagedSparseSet<T*> entity_to_component;

      /// Track component count for statistics
      size_t component_count;

    public:
      /**
       * @brief Constructs a managed component array.
       * @param initial_page_size Size of initial memory page for the pool.
       */
      explicit ManagedComponentArray(size_t initial_page_size = Memory::DEFAULT_PAGE_SIZE)
        : pool(initial_page_size)
        , component_count(0)
      {
      }

      /**
       * @brief Destructor - cleans up all components.
       */
      ~ManagedComponentArray() override {
        // Destruct all components
        for (size_t i = 0; i < entity_to_component.size(); ++i) {
          T* component = entity_to_component.at(i);
          if (component) {
            pool.destruct(component);
          }
        }
      }

      // Disable copy
      ManagedComponentArray(const ManagedComponentArray&) = delete;
      ManagedComponentArray& operator=(const ManagedComponentArray&) = delete;

      // Enable move
      ManagedComponentArray(ManagedComponentArray&&) = default;
      ManagedComponentArray& operator=(ManagedComponentArray&&) = default;

      /**
       * @brief Adds a component to an entity.
       *
       * Uses the pool allocator to allocate memory, then uses placement new
       * to construct the component.
       *
       * @param entity The entity ID to add the component to.
       * @param component The component data to copy.
       */
      void add_component(EntityID entity, const T& component) {
        // Remove existing component if present
        if (entity_to_component.contains(entity)) {
          T* existing = *entity_to_component.get(entity);
          if (existing) {
            pool.destruct(existing);
            --component_count;
          }
        }

        // Allocate and construct new component using placement new
        T* new_component = pool.construct(component);
        if (new_component) {
          entity_to_component.insert(entity, new_component);
          ++component_count;
        }
      }

      /**
       * @brief Adds a component to an entity (move version).
       * @param entity The entity ID to add the component to.
       * @param component The component data to move.
       */
      void add_component(EntityID entity, T&& component) {
        if (entity_to_component.contains(entity)) {
          T* existing = *entity_to_component.get(entity);
          if (existing) {
            pool.destruct(existing);
            --component_count;
          }
        }

        T* new_component = pool.construct(std::move(component));
        if (new_component) {
          entity_to_component.insert(entity, new_component);
          ++component_count;
        }
      }

      /**
       * @brief Emplaces a component for an entity (constructs in-place).
       * @tparam Args Constructor argument types.
       * @param entity The entity ID.
       * @param args Arguments forwarded to T's constructor.
       */
      template<typename... Args>
      void emplace_component(EntityID entity, Args&&... args) {
        if (entity_to_component.contains(entity)) {
          T* existing = *entity_to_component.get(entity);
          if (existing) {
            pool.destruct(existing);
            --component_count;
          }
        }

        T* new_component = pool.construct(std::forward<Args>(args)...);
        if (new_component) {
          entity_to_component.insert(entity, new_component);
          ++component_count;
        }
      }

      /**
       * @brief Clones a component from one entity to another.
       * @param source The source entity ID.
       * @param destination The destination entity ID.
       */
      void clone_component(EntityID source, EntityID destination) override {
        T** source_ptr = entity_to_component.get(source);
        if (source_ptr && *source_ptr) {
          add_component(destination, **source_ptr);
        }
      }

      /**
       * @brief Gets a component pointer for an entity.
       * @param entity The entity ID.
       * @return Pointer to the component, or nullptr if not found.
       */
      T* get_component(EntityID entity) {
        T** ptr = entity_to_component.get(entity);
        return ptr ? *ptr : nullptr;
      }

      /**
       * @brief Gets a const component pointer for an entity.
       * @param entity The entity ID.
       * @return Const pointer to the component, or nullptr if not found.
       */
      const T* get_component(EntityID entity) const {
        const T* const* ptr = entity_to_component.get(entity);
        return ptr ? *ptr : nullptr;
      }

      /**
       * @brief Checks if an entity has this component.
       * @param entity The entity ID.
       * @return true if the entity has the component.
       */
      bool has_component(EntityID entity) const {
        return entity_to_component.contains(entity);
      }

      /**
       * @brief Removes a component from an entity.
       *
       * Calls the component's destructor and returns memory to the pool.
       * The memory is NOT freed to the OS - it's added to the free list.
       *
       * @param entity The entity ID.
       */
      void remove_entity(EntityID entity) override {
        if (!entity_to_component.contains(entity)) return;

        T** ptr = entity_to_component.get(entity);
        if (ptr && *ptr) {
          pool.destruct(*ptr);
          --component_count;
        }
        entity_to_component.erase(entity);
      }

      /**
       * @brief Returns the type ID for this component type.
       */
      ComponentTypeID get_type_id() const override {
        return ComponentTypeManager::get_type_id<T>();
      }

      /**
       * @brief Checks if the component type is reflectable.
       */
      bool is_reflectable() const override {
        return ReflectionLibrary::is_reflectable_v<T>;
      }

      /**
       * @brief Returns the type name as a string.
       */
      std::string get_type_name() const override {
        if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
          return std::string(ReflectionLibrary::class_name<T>);
        }
        else {
          return "Unknown";
        }
      }

      /**
       * @brief Converts a component to a GenericValue for serialization.
       */
      ReflectionLibrary::GenericValue to_generic_value(EntityID entity) const override {
        const T* component = get_component(entity);
        if (component != nullptr) {
          if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
            return ReflectionLibrary::to_generic_value(*component);
          }
        }
        return ReflectionLibrary::GenericValue();
      }

      /**
       * @brief Adds a component from a GenericValue.
       */
      bool add_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) override {
        if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
          std::optional<T> component_opt = ReflectionLibrary::from_generic_value<T>(value);
          if (component_opt) {
            add_component(entity, std::move(component_opt.value()));
            return true;
          }
        }
        return false;
      }

      /**
       * @brief Sets an existing component's data from a GenericValue.
       */
      bool set_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value) override {
        T* component = get_component(entity);
        if (component == nullptr) {
          return false;
        }

        if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
          std::optional<T> component_opt = ReflectionLibrary::from_generic_value<T>(value);
          if (component_opt) {
            *component = std::move(component_opt.value());
            return true;
          }
        }
        return false;
      }

      /**
       * @brief Adds a default-constructed component to an entity.
       */
      void add_default_component(EntityID entity) override {
        if constexpr (std::is_default_constructible_v<T>) {
          emplace_component(entity);
        }
      }

      /**
       * @brief Creates an empty component array of the same type.
       */
      ManagedUniquePtr<IManagedComponentArray> create_empty() const override {
        return make_managed<ManagedComponentArray<T>>();
      }

      // ============================================================================
      // Memory Manager Specific Methods
      // ============================================================================

      /**
       * @brief Returns the total memory usage of this component array.
       */
      size_t get_memory_usage() const override {
        return pool.get_total_memory();
      }

      /**
       * @brief Returns the number of active allocations.
       */
      size_t get_allocation_count() const override {
        return pool.get_active_allocations();
      }

      /**
       * @brief Resets the memory pool, returning all memory to free list.
       *
       * WARNING: This invalidates all component pointers!
       * Only call this when unloading a level.
       */
      void reset_memory() override {
        entity_to_component.clear();
        pool.reset();
        component_count = 0;
      }

      /**
       * @brief Pre-allocates memory pages for expected component count.
       * @param expected_count Expected number of components.
       */
      void reserve(size_t expected_count) {
        size_t blocks_per_page = pool.get_pool().get_blocks_per_page();
        size_t pages_needed = (expected_count + blocks_per_page - 1) / blocks_per_page;
        if (pages_needed > pool.get_page_count()) {
          pool.reserve_pages(pages_needed - pool.get_page_count());
        }
      }

      // ============================================================================
      // Iteration Support
      // ============================================================================

      /**
       * @brief Iterator for iterating over all components.
       */
      class Iterator {
      private:
        ManagedSparseSet<T*>* set;
        size_t index;

      public:
        Iterator(ManagedSparseSet<T*>* s, size_t i) : set(s), index(i) {}

        T& operator*() { return *set->at(index); }
        T* operator->() { return set->at(index); }

        Iterator& operator++() { ++index; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; ++index; return tmp; }

        bool operator==(const Iterator& other) const { return index == other.index; }
        bool operator!=(const Iterator& other) const { return index != other.index; }

        EntityID entity_id() const { return set->key_at(index); }
      };

      Iterator begin() { return Iterator(&entity_to_component, 0); }
      Iterator end() { return Iterator(&entity_to_component, entity_to_component.size()); }

      /**
       * @brief Returns the number of components stored.
       */
      size_t size() const { return component_count; }

      /**
       * @brief Checks if the array is empty.
       */
      bool empty() const { return component_count == 0; }
    };

  } // namespace ECS
} // namespace Engine
