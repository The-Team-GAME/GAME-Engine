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
      components.insert(entity, component);
    }

    template<typename T>
    void ComponentArray<T>::clone_component(EntityID source, EntityID destination) {
      const T* source_component = components.get(source);
      if (source_component != nullptr) {
        components.insert(destination, *source_component); // Assumes T is copy-constructible
      }
    }

    template<typename T>
    T* ComponentArray<T>::get_component(EntityID entity) {
      return components.get(entity);
    }

    template<typename T>
    bool ComponentArray<T>::has_component(EntityID entity) const {
      return components.contains(entity);
    }

    template<typename T>
    void ComponentArray<T>::remove_entity(EntityID entity) {
      components.erase(entity);
    }

    template<typename T>
    inline ComponentTypeID ComponentArray<T>::get_type_id() const
    {
      return ComponentTypeManager::get_type_id<T>();
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
      const T* component = components.get(entity);
      if (component != nullptr) {
        if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
          return ReflectionLibrary::to_generic_value(*component);
        }
      }
      return ReflectionLibrary::GenericValue();
    }

    template<typename T>
    inline bool ComponentArray<T>::add_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value)
    {
      if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
        // Both components with and without reflectors use from_generic_value<T>
        // because REFLECT_CUSTOM creates ReflectionInfo<T> with Reflector fields
        std::optional<T> component_opt = ReflectionLibrary::from_generic_value<T>(value);
        if (component_opt) {
          add_component(entity, std::move(component_opt.value()));
          return true;
        }
      }
      return false;
    }

    template<typename T>
    inline bool ComponentArray<T>::set_component_from_generic_value(EntityID entity, const ReflectionLibrary::GenericValue& value)
    {
      // Check if the entity has this component
      T* component = components.get(entity);
      if (component == nullptr) {
        return false; // Entity doesn't have this component
      }

      if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
        // Both components with and without reflectors use from_generic_value<T>
        // because REFLECT_CUSTOM creates ReflectionInfo<T> with Reflector fields
        std::optional<T> component_opt = ReflectionLibrary::from_generic_value<T>(value);
        if (component_opt) {
          *component = std::move(component_opt.value());
          return true;
        }
      }
      return false;
    }

    template<typename T>
    inline void ComponentArray<T>::add_default_component(EntityID entity)
    {
      if constexpr (std::is_default_constructible_v<T>) {
        Engine::ECS::add_component<T>(entity, T{});
      }
    }

    template<typename T>
    auto ComponentArray<T>::begin() {
      return components.data_begin();
    }

    template<typename T>
    auto ComponentArray<T>::end() {
      return components.data_end();
    }

    template<typename T>
    auto ComponentArray<T>::begin() const {
      return components.data_begin();
    }

    template<typename T>
    auto ComponentArray<T>::end() const {
      return components.data_end();
    }

    template<typename T>
    inline std::unique_ptr<IComponentArray> ComponentArray<T>::create_empty() const
    {
      return std::make_unique<ComponentArray<T>>();
    }

  } // namespace ECS
} // namespace Engine