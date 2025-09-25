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
    auto* posA = world->getComponent<Components::Position>(entityId);
    auto* colA = world->getComponent<Components::Collider>(entityId);
    if (!posA || !colA) return;

    for (auto otherId : world->getEntitiesWith<Components::Collider>()) {
      if (otherId <= entityId) continue; // Avoid duplicates/self

      auto* posB = world->getComponent<Components::Position>(otherId);
      auto* colB = world->getComponent<Components::Collider>(otherId);
      if (!posB || !colB) continue;

      Math::Vec2D overlap;
      if (checkAABBCollision(*posA, *colA, *posB, *colB, overlap)) {
        // Store collision in the queue
				std::cout << "Collision detected between Entity " << entityId << " and Entity " << otherId << "\n";
      }
      else
      {
        std::cout << "No collision between Entity " << entityId << " and Entity " << otherId << "\n";
      }
    }
  }

private:
  bool checkAABBCollision(const Components::Position& aPos,
    const Components::Collider& aCol,
    const Components::Position& bPos,
    const Components::Collider& bCol,
    Math::Vec2D& outOverlap) {
    float dx = bPos.x - aPos.x;
    float px = (aCol.halfWidth + bCol.halfWidth) - std::abs(dx);
    if (px <= 0) return false;

    float dy = bPos.y - aPos.y;
    float py = (aCol.halfHeight + bCol.halfHeight) - std::abs(dy);
    if (py <= 0) return false;

    // Overlap vector points from A to B
    outOverlap.x = (dx > 0) ? px : -px;
    outOverlap.y = (dy > 0) ? py : -py;

    return true;
  }
};