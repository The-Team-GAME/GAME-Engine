#pragma once


#include "ECS/System.h"
#include "BasicComponents.h"
#include "Core.h"
#include "Vector2D.h"

class CollisionSystem : public ECS::System {
public:
  CollisionSystem() {
    requireComponents<Components::Collider, Components::Position>();
  }

  void updateEntity(ECS::EntityID entityId) override {
    // Get this entity's components
    auto* posA = world->getComponent<Components::Position>(entityId);
    auto* colA = world->getComponent<Components::Collider>(entityId);

    if (!posA || !colA) return;

    // Iterate over other entities that have a collider
    for (auto otherId : world->getEntitiesWith<Components::Collider>()) {
      if (otherId == entityId) continue; // Skip self

      auto* posB = world->getComponent<Components::Position>(otherId);
      auto* colB = world->getComponent<Components::Collider>(otherId);

      if (!posB || !colB) continue; // Skip if missing Position/Collider

      // Run AABB collision check
      if (checkAABBCollision(*posA, *colA, *posB, *colB)) {
        // Handle collision response here
        // e.g. set a flag, apply damage, etc.
        std::cout << "Collision detected between "
          << entityId << " and " << otherId << "\n";
      }
      else 
        {
        // No collision
        std::cout << "No collision between " << entityId << " and " << otherId << "\n";
			}
    }
		//std::cout << "running Collision system on entity " << entityId << "\n";
  }

  bool checkAABBCollision(const Components::Position& aPos,
    const Components::Collider& aCol,
    const Components::Position& bPos,
    const Components::Collider& bCol) {
    return std::abs(aPos.x - bPos.x) <= (aCol.halfWidth + bCol.halfWidth) &&
      std::abs(aPos.y - bPos.y) <= (aCol.halfHeight + bCol.halfHeight);
  }
};