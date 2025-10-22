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
    void ComponentArray<T>::add_component(EntityID entity, T component) {
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
    size_t ComponentArray<T>::size() const {
      return components.size();
    }

    template<typename T>
    bool ComponentArray<T>::is_reflectable() const {
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
    void ComponentArray<T>::serialize_component(EntityID entity, rapidjson::Value& json_val, rapidjson::Document::AllocatorType& allocator) const {
      auto it = components.find(entity);
      if (it != components.end()) {
        if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
          T component_copy = it->second;
          if constexpr (ReflectionLibrary::has_reflector_v<T>) {
            auto reflector = ReflectionLibrary::ReflectionInfo<T>::reflect(component_copy);
            ReflectionLibrary::serialize_obj(reflector, json_val, allocator);
          }
          else {
            ReflectionLibrary::serialize_obj(component_copy, json_val, allocator);
          }
        }
      }
    }

    template<typename T>
    bool ComponentArray<T>::deserialize_and_add_component(EntityID entity, const rapidjson::Value& json_val) {
      if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
        if constexpr (ReflectionLibrary::has_reflector_v<T>) {
          // Component has custom reflector
          typename T::Reflector reflector{};
          if (ReflectionLibrary::deserialize_obj(reflector, json_val)) {
            T component = ReflectionLibrary::ReflectionInfo<T>::create(reflector);
            add_component(entity, std::move(component));
            return true;
          }
        }
        else {
          // Component uses direct reflection
          T component{};
          if (ReflectionLibrary::deserialize_obj(component, json_val)) {
            add_component(entity, std::move(component));
            return true;
          }
        }
      }
      return false;
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