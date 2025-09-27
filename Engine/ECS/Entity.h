#pragma once
#include "Types.h"

namespace ECS {

  class World; // Forward declaration

  // Entity handle for clean API
  class Entity {
  private:
    EntityID id;
    World* world;

  public:
    Entity(EntityID id = INVALID_ENTITY, World* world = nullptr) : id(id), world(world) {}

    EntityID getId() const { return id; }
    bool isValid() const { return id != INVALID_ENTITY; }

    template<typename T, typename... Args>
    Entity& addComponent(Args&&... args);

    template<typename T>
    Entity& removeComponent();

    template<typename T>
    T* getComponent();

    template<typename T>
    const T* getComponent() const;

    template<typename T>
    bool hasComponent() const;

    template<typename... T>
    bool hasComponents() const;

    void destroy();

    bool operator==(const Entity& other) const { return id == other.id; }
    bool operator!=(const Entity& other) const { return id != other.id; }
  };

} // namespace ECS