#pragma once
#include "ComponentArray.h"

namespace ECS {

  template<typename T>
  void ComponentArray<T>::addComponent(EntityID entity, T component) {
    components.emplace(entity, std::move(component));
  }

  template<typename T>
  void ComponentArray<T>::removeComponent(EntityID entity) {
    components.erase(entity);
  }

  template<typename T>
  void ComponentArray<T>::cloneComponent(EntityID source, EntityID destination) {
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
  bool ComponentArray<T>::hasComponent(EntityID entity) const {
    return components.find(entity) != components.end();
  }

  template<typename T>
  void ComponentArray<T>::removeEntity(EntityID entity) {
    removeComponent(entity);
  }

  template<typename T>
  size_t ComponentArray<T>::size() const {
    return components.size();
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