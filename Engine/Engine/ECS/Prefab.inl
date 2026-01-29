//-----------------------------------------------------------------------------------
// file:    Prefab.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for the PrefabImpl and Prefab
//          classes. Provides inline implementations for adding, removing, and accessing
//          components in prefab templates.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "Prefab.h"
#include "ComponentManager.h"
#include "ManagedComponentArray.h"

namespace Engine {
  namespace ECS {

    // ============================================================================
    // PrefabImpl Template Implementation
    // ============================================================================

    template<typename T, typename... Args>
      requires std::is_constructible_v<T, Args...>
    PrefabImpl& PrefabImpl::add_component(Args&&... args) {
      ComponentTypeID type_id = ComponentTypeManager::get_type_id<T>();

      // Create component array if it doesn't exist
      if (!component_storage.find(type_id)) {
        component_storage.insert(type_id, ManagedUniquePtr<IManagedComponentArray>::make<ManagedComponentArray<T>>());
      }

      // Add component using a dummy entity ID (0)
      auto* array = static_cast<ManagedComponentArray<T>*>(component_storage.find(type_id)->get());
      array->add_component(0, T(std::forward<Args>(args)...));

      // Update signature
      signature.set(type_id);

      return *this;
    }

    template<typename T>
    PrefabImpl& PrefabImpl::remove_component() {
      ComponentTypeID type_id = ComponentTypeManager::get_type_id<T>();

      if (component_storage.find(type_id)) {
        component_storage.erase(type_id);
        signature.reset(type_id);
      }

      return *this;
    }

    template<typename T>
    T* PrefabImpl::get_component() {
      ComponentTypeID type_id = ComponentTypeManager::get_type_id<T>();

      auto* ptr = component_storage.find(type_id);
      if (ptr) {
        auto* array = static_cast<ManagedComponentArray<T>*>(ptr->get());
        return array->get_component(0); // Using dummy entity ID 0
      }

      return nullptr;
    }

    template<typename T>
    const T* PrefabImpl::get_component() const {
      ComponentTypeID type_id = ComponentTypeManager::get_type_id<T>();

      auto* ptr = component_storage.find(type_id);
      if (ptr) {
        auto* array = static_cast<const ManagedComponentArray<T>*>(ptr->get());
        return array->get_component(0); // Using dummy entity ID 0
      }

      return nullptr;
    }

    template<typename T>
    bool PrefabImpl::has_component() const {
      ComponentTypeID type_id = ComponentTypeManager::get_type_id<T>();
      return signature.test(type_id);
    }

    template<typename... T>
    bool PrefabImpl::has_components() const {
      return (has_component<T>() && ...);
    }

    // ============================================================================
    // Prefab Handle Template Implementation
    // ============================================================================

    template<typename T, typename... Args>
      requires std::is_constructible_v<T, Args...>
    Prefab& Prefab::add_component(Args&&... args) {
      if (impl) {
        impl->add_component<T>(std::forward<Args>(args)...);
      }
      return *this;
    }

    template<typename T>
    Prefab& Prefab::remove_component() {
      if (impl) {
        impl->remove_component<T>();
      }
      return *this;
    }

    template<typename T>
    T* Prefab::get_component() {
      return impl ? impl->get_component<T>() : nullptr;
    }

    template<typename T>
    const T* Prefab::get_component() const {
      return impl ? impl->get_component<T>() : nullptr;
    }

    template<typename T>
    bool Prefab::has_component() const {
      return impl ? impl->has_component<T>() : false;
    }

    template<typename... T>
    bool Prefab::has_components() const {
      return impl ? impl->has_components<T...>() : false;
    }

  } // namespace ECS
} // namespace Engine