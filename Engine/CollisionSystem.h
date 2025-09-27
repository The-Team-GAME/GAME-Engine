#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include <iostream>

class CollisionSystem : public ECS::System {
public:
  CollisionSystem() {
    requireComponents<Components::Collider, Components::Position>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* posA = entity.getComponent<Components::Position>( );
    auto* colA = entity.getComponent<Components::Collider>( );
    if (!posA || !colA) return;

    // Clear previous penetration
    colA->penetration = { 0.0f, 0.0f };

    for (ECS::Entity otherEntity : world->getEntitiesWith<Components::Collider>()) {
      auto otherId = otherEntity.getId();
      if (otherId <= entity.getId()) continue; // skip self + duplicates

      auto* posB = world->getComponent<Components::Position>(otherId);
      auto* colB = world->getComponent<Components::Collider>(otherId);
      if (!posB || !colB) continue;

      Math::Vec2D overlap;
      if (checkAABBCollision(*posA, *colA, *posB, *colB, overlap)) {
        /* std::cout << "Collision detected between Entity "
           << entityId << " and Entity " << otherId << "\n";*/

           // Set penetration vectors
        colA->penetration = -overlap;
        colB->penetration = overlap;
      }
    }
  }

private:
  bool checkAABBCollision(const Components::Position& aPos,
    const Components::Collider& aCol,
    const Components::Position& bPos,
    const Components::Collider& bCol,
    Math::Vec2D& outOverlap)
  {
    float dx = bPos.x - aPos.x;
    float px = (aCol.halfWidth + bCol.halfWidth) - std::abs(dx);
    if (px <= 0) return false;

    float dy = bPos.y - aPos.y;
    float py = (aCol.halfHeight + bCol.halfHeight) - std::abs(dy);
    if (py <= 0) return false;

    // Resolve along shallowest axis to prevent diagonal tunneling
    if (px < py) {
      outOverlap = { (dx > 0) ? px : -px, 0.0f };
    }
    else {
      outOverlap = { 0.0f, (dy > 0) ? py : -py };
    }

    return true;
  }
};
