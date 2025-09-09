#pragma once

#include "Types.h"
#include <unordered_map>
#include <memory>

namespace ECS {

  // Component storage interface
  class IComponentArray {
  public:
    virtual ~IComponentArray() = default;
    virtual void removeEntity(EntityID entity) = 0;
    virtual size_t size() const = 0;
    virtual void cloneComponent(EntityID source, EntityID destination) = 0;
  };

  // Type-safe component storage
  template<typename T>
  class ComponentArray : public IComponentArray {
  private:
    std::unordered_map<EntityID, T> components;

  public:
    void addComponent(EntityID entity, T component) {
      components.emplace(entity, std::move(component));
    }

    void removeComponent(EntityID entity) {
      components.erase(entity);
    }

    void cloneComponent(EntityID source, EntityID destination) override {
      auto it = components.find(source);
      if (it != components.end()) {
        components.emplace(destination, it->second); // Assumes T is copy-constructible
      }
    }

    T* getComponent(EntityID entity) {
      auto it = components.find(entity);
      return it != components.end() ? &it->second : nullptr;
    }

    bool hasComponent(EntityID entity) const {
      return components.find(entity) != components.end();
    }

    void removeEntity(EntityID entity) override {
      removeComponent(entity);
    }

    size_t size() const override {
      return components.size();
    }

    auto begin() { return components.begin(); }
    auto end() { return components.end(); }
    auto begin() const { return components.begin(); }
    auto end() const { return components.end(); }
  };

} // namespace ECS