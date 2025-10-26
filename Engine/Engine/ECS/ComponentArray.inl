//-----------------------------------------------------------------------------------
// file:    ComponentArray.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for the ComponentArray class.
//          Provides inline implementations for adding, removing, cloning, and accessing
//          components by entity ID.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "ComponentArray.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../Reflection/ReflectionLibrary/Json.h"

namespace Engine {
  namespace ECS {

    template<typename T>
    void ComponentArray<T>::add_component(EntityID entity, const T& component) {
      components.emplace(entity, std::move(component));
    }

    template<typename T>
    void ComponentArray<T>::clone_component(EntityID source, EntityID destination) {
      auto it = components.find(source);
      if (it != components.end()) {
        components.emplace(destination, it->second); // Assumes T is copy-constructible
      }
    }

    template<typename T>
    T* ComponentArray<T>::get_component(EntityID entity) {
      auto it = components.find(entity);
      return it != components.end() ? &it->second : nullptr;
    }

    template<typename T>
    bool ComponentArray<T>::has_component(EntityID entity) const {
      return components.find(entity) != components.end();
    }

    template<typename T>
    void ComponentArray<T>::remove_entity(EntityID entity) {
      components.erase(entity);
    }

    template<typename T>
    inline bool ComponentArray<T>::is_reflectable() const
    {
      return ReflectionLibrary::is_reflectable_v<T>;
    }

    template<typename T>
    std::string ComponentArray<T>::get_type_name() const {
      if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
        return std::string(ReflectionLibrary::class_name<T>);
      }
      else {
        return "Unknown";
      }
    }

    template<typename T>
    inline ReflectionLibrary::GenericValue ComponentArray<T>::to_generic_value(EntityID entity) const
    {
      auto it = components.find(entity);
      if (it != components.end()) {
        if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
          return ReflectionLibrary::to_generic_value(it->second);
        }
      }
      return ReflectionLibrary::GenericValue();
    }

    template<typename T>
    inline bool ComponentArray<T>::add_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value)
    {
      if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
        if constexpr (ReflectionLibrary::has_reflector_v<T>) {
          // Component has custom reflector
          std::optional<typename T::Reflector> reflector_opt = ReflectionLibrary::from_generic_value<typename T::Reflector>(value);
          if (reflector_opt) {
            T component = ReflectionLibrary::ReflectionInfo<T>::create(reflector_opt.value());
            add_component(entity, std::move(component));
            return true;
          }
        }
        else {
          // Component uses direct reflection
          std::optional<T> component_opt = ReflectionLibrary::from_generic_value<T>(value);
          if (component_opt) {
            add_component(entity, std::move(component_opt.value()));
            return true;
          }
        }
      }
      return false;
    }

    template<typename T>
    inline bool ComponentArray<T>::set_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value)
    {
      // Check if the entity has this component
      auto it = components.find(entity);
      if (it == components.end()) {
        return false; // Entity doesn't have this component
      }

      if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
        if constexpr (ReflectionLibrary::has_reflector_v<T>) {
          // Component has custom reflector
          std::optional<typename T::Reflector> reflector_opt = ReflectionLibrary::from_generic_value<typename T::Reflector>(value);
          if (reflector_opt) {
            it->second = ReflectionLibrary::ReflectionInfo<T>::create(reflector_opt.value());
            return true;
          }
        }
        else {
          // Component uses direct reflection
          std::optional<T> component_opt = ReflectionLibrary::from_generic_value<T>(value);
          if (component_opt) {
            it->second = std::move(component_opt.value());
            return true;
          }
        }
      }
      return false;
    }

    template<typename T>
    inline void ComponentArray<T>::add_default_component(EntityID entity)
    {
      if constexpr (std::is_default_constructible_v<T>) {
        components.emplace(entity, T{});
      }
    }

    template<typename T>
    auto ComponentArray<T>::begin() {
      return components.begin();
    }

    template<typename T>
    auto ComponentArray<T>::end() {
      return components.end();
    }

    template<typename T>
    auto ComponentArray<T>::begin() const {
      return components.begin();
    }

    template<typename T>
    auto ComponentArray<T>::end() const {
      return components.end();
    }

  } // namespace ECS
} // namespace Engine