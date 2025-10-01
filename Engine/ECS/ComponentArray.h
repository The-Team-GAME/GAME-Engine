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
    void addComponent(EntityID entity, T component);

    void removeComponent(EntityID entity);

    void cloneComponent(EntityID source, EntityID destination) override;

    T* get_component(EntityID entity);

    bool hasComponent(EntityID entity) const;

    void removeEntity(EntityID entity) override;

    size_t size() const override;

    auto begin();
    auto end();
    auto begin() const;
    auto end() const;
  };

} // namespace ECS