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
    Entity(EntityID id = INVALID_ENTITY, World* world = nullptr);

    EntityID getId() const;
    bool isValid() const;

    template<typename T, typename... Args>
    Entity& add_component(Args&&... args);

    template<typename T>
    Entity& remove_component();

    template<typename T>
    T* get_component();

    template<typename T>
    const T* get_component() const;

    template<typename T>
    bool has_component() const;

    template<typename... T>
    bool has_components() const;

    void destroy();

    bool operator==(const Entity& other) const;
    bool operator!=(const Entity& other) const;
  };

} // namespace ECS

