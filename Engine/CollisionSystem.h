#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"
#include <iostream>

class CollisionSystem : public ECS::System {
public:
  CollisionSystem() {
    requireComponents<Components::Collider, Components::Position, Components::RigidBody>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* posA = entity.getComponent<Components::Position>( );
    auto* colA = entity.getComponent<Components::Collider>( );
    auto* rbA = entity.getComponent<Components::RigidBody>();

    if (!posA || !colA || !rbA) return;

    // Clear previous penetration
    colA->penetration = { 0.0f, 0.0f };
    
    float footOffset = 0.01f; // small distance below collider
    Math::Vec2D footPos = { posA->x, posA->y - colA->halfHeight - footOffset };

    for (ECS::Entity otherEntity : world->getEntitiesWith<Components::Collider>()) {
      auto otherId = otherEntity.getId();
      if (otherId == entity.getId()) continue; // skip self + duplicates

      auto* posB = world->getComponent<Components::Position>(otherId);
      auto* colB = world->getComponent<Components::Collider>(otherId);
      auto* rbB = world->getComponent<Components::RigidBody>(otherId); // nullptr if static


      if (!posB || !colB) continue;

      // Try dynamic collision first (if both have rigidbodies and neither is static)
      bool dynamicCollisionHandled = false;

      if (!colA->isStatic && !colB->isStatic && rbA && rbB) {
        // Dynamic vs Dynamic collision
        Math::Vec2D velB = rbB->velocity;
        Math::Vec2D normal;
        float t = sweptAABB(*posA, *rbA, *colA, *posB, velB, *colB, normal);

        if (t < 0.01666666666f) {
          // Stop velocity along collision normal
          if (normal.x != 0) rbA->velocity.x = 0;
          if (normal.y != 0) rbA->velocity.y = 0;

          std::cout << "Dynamic collision between "
            << entity.getId() << " and " << otherEntity.getId()
            << " normal=(" << normal.x << ", " << normal.y << ")\n";

          dynamicCollisionHandled = true;
        }
      }

      // Fall back to static collision check if no dynamic collision occurred
      if (!dynamicCollisionHandled) {
        Math::Vec2D overlap;
        if (checkAABBCollision(*posA, *colA, *posB, *colB, overlap)) {
    /*      std::cout << "Static collision between "
            << entity.getId() << " and " << otherEntity.getId()
            << " overlap=(" << overlap.x << ", " << overlap.y << ")\n";*/

          // Store penetration based on which entity is static
          if (colA->isStatic) {
            colB->penetration = overlap;
          }
          if (colB->isStatic) {
            colA->penetration = -overlap;
          }

          // If both are dynamic (no static collision occurred above), store for both
          if (!colA->isStatic && !colB->isStatic) {
            colA->penetration = -overlap;
            colB->penetration = overlap;
          }
        }
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

  float sweptAABB(const Components::Position& aPos, const Components::RigidBody& aRb, const Components::Collider& aCol,
    const Components::Position& bPos, const Math::Vec2D& bVel, const Components::Collider& bCol,
    Math::Vec2D& normal)
  {
    float vx = aRb.velocity.x - bVel.x;
    float vy = aRb.velocity.y - bVel.y;

    float xInvEntry, yInvEntry;
    float xInvExit, yInvExit;

    if (vx > 0.0f) {
      xInvEntry = (bPos.x - bCol.halfWidth) - (aPos.x + aCol.halfWidth);
      xInvExit = (bPos.x + bCol.halfWidth) - (aPos.x - aCol.halfWidth);
    }
    else {
      xInvEntry = (bPos.x + bCol.halfWidth) - (aPos.x - aCol.halfWidth);
      xInvExit = (bPos.x - bCol.halfWidth) - (aPos.x + aCol.halfWidth);
    }

    if (vy > 0.0f) {
      yInvEntry = (bPos.y - bCol.halfHeight) - (aPos.y + aCol.halfHeight);
      yInvExit = (bPos.y + bCol.halfHeight) - (aPos.y - aCol.halfHeight);
    }
    else {
      yInvEntry = (bPos.y + bCol.halfHeight) - (aPos.y - aCol.halfHeight);
      yInvExit = (bPos.y - bCol.halfHeight) - (aPos.y + aCol.halfHeight);
    }

    float xEntry = (vx == 0.0f) ? -INFINITY : xInvEntry / vx;
    float xExit = (vx == 0.0f) ? INFINITY : xInvExit / vx;
    float yEntry = (vy == 0.0f) ? -INFINITY : yInvEntry / vy;
    float yExit = (vy == 0.0f) ? INFINITY : yInvExit / vy;

    float entryTime = std::max(xEntry, yEntry);
    float exitTime = std::min(xExit, yExit);

    if (entryTime > exitTime || (xEntry < 0.0f && yEntry < 0.0f) || entryTime > 1.0f) {
      normal = { 0.0f, 0.0f };
      return 1.0f; // no collision this frame
    }

    if (xEntry > yEntry) normal = (xInvEntry < 0.0f) ? Math::Vec2D{ 1.0f,0.0f } : Math::Vec2D{ -1.0f,0.0f };
    else                 normal = (yInvEntry < 0.0f) ? Math::Vec2D{ 0.0f,1.0f } : Math::Vec2D{ 0.0f,-1.0f };

    return entryTime;
  }
};
