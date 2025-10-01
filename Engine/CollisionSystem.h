#pragma once


#include "ECS/System.h" 
#include "BasicComponents.h"
#include <iostream>
#include <algorithm>

class CollisionSystem : public ECS::System {
public:
  CollisionSystem() {
    require_components<Components::Collider, Components::Position, Components::RigidBody>();
  }

  void updateEntity(ECS::Entity& entity) override {
    auto* posA = entity.get_component<Components::Position>();
    auto* colA = entity.get_component<Components::Collider>();
    auto* rbA = entity.get_component<Components::RigidBody>();

    if (!posA || !colA || !rbA) return;

    const float dt = Engine::Time::get_fixed_dt();
    float footOffset = 0.01f;
    Math::Vec2D footPos = { posA->X(), posA->Y() - colA->halfHeight - footOffset};

    for (ECS::Entity otherEntity : world->getEntitiesWith<Components::Collider>()) {
      auto otherId = otherEntity.getId();
      if (otherId <= entity.getId()) continue; // Skip self + duplicates

      auto* posB = world->get_component<Components::Position>(otherId);
      auto* colB = world->get_component<Components::Collider>(otherId);
      auto* rbB = world->get_component<Components::RigidBody>(otherId); // nullptr if static

      if (!posB || !colB) continue;

      bool handledDynamic = false;

      // --- Dynamic vs Dynamic collision ---
      if (!colA->isStatic && !colB->isStatic && rbA && rbB) {
        float tFirst = 0.0f;
        Math::Vec2D normal;
        if (sweptAABB_Dynamic(*posA, *rbA, *colA, *posB, rbB->velocity, *colB, dt, tFirst, normal)) {
          if (tFirst <= dt) {
            colA->dynamicCollisionTime = tFirst;
            colA->dynamicNormal = normal;

            colB->dynamicCollisionTime = tFirst;
            colB->dynamicNormal = -normal;

            /*std::cout << "Dynamic collision between " << entity.getId() << " and " << otherEntity.getId()
              << " normal=(" << normal.x << ", " << normal.y << ")\n";*/

            handledDynamic = true;
          }
        }
      }

      // --- Static collision fallback ---
      if (!handledDynamic) {
        Math::Vec2D overlap;
        if (checkAABBCollision(*posA, *colA, *posB, *colB, overlap)) {
          storePenetration(*colA, *colB, overlap);
        }
      }
    }
  }

private:
  bool checkAABBCollision(const Components::Position& aPos, const Components::Collider& aCol,
    const Components::Position& bPos, const Components::Collider& bCol,
    Math::Vec2D& outOverlap)
  {
    float dx = bPos.X() - aPos.X();
    float px = (aCol.halfWidth + bCol.halfWidth) - std::abs(dx);
    if (px <= 0) return false;

    float dy = bPos.Y() - aPos.Y();
    float py = (aCol.halfHeight + bCol.halfHeight) - std::abs(dy);
    if (py <= 0) return false;

    if (px < py) outOverlap = { (dx > 0) ? px : -px, 0.0f };
    else         outOverlap = { 0.0f, (dy > 0) ? py : -py };

    return true;
  }

  void storePenetration(Components::Collider& aCol, Components::Collider& bCol, const Math::Vec2D& overlap) {
    if (aCol.isStatic) bCol.penetration = overlap;
    if (bCol.isStatic) aCol.penetration = overlap * -1;

    if (!aCol.isStatic && !bCol.isStatic) {
      aCol.penetration = overlap * -1;
      bCol.penetration = overlap;
    }
  }

  bool sweptAABB_Dynamic(const Components::Position& aPos, const Components::RigidBody& aRb, const Components::Collider& aCol,
    const Components::Position& bPos, const Math::Vec2D& bVel, const Components::Collider& bCol,
    float dt, float& firstTimeOfCollision, Math::Vec2D& normal)
  {
    float vx = aRb.velocity.X() - bVel.X();
    float vy = aRb.velocity.Y() - bVel.Y();

    float tFirst = 0.0f;
    float tLast = dt;

    auto processAxis = [&](float aMin, float aMax, float bMin, float bMax, float v) -> bool {
      if (v < 0.0f) {
        if (aMax < bMin) tFirst = std::max((aMax - bMin) / v, tFirst);
        if (aMin < bMax) tLast = std::min((aMin - bMax) / v, tLast);
        if (aMin > bMax) return false;
      }
      else if (v > 0.0f) {
        if (aMin > bMax) tFirst = std::max((aMin - bMax) / v, tFirst);
        if (aMax > bMin) tLast = std::min((aMax - bMin) / v, tLast);
        if (aMax < bMin) return false;
      }
      else {
        if (aMax < bMin || aMin > bMax) return false;
      }
      return true;
      };

    if (!processAxis(aPos.X() - aCol.halfWidth, aPos.X() + aCol.halfWidth, bPos.X() - bCol.halfWidth, bPos.X() + bCol.halfWidth, vx))
      return false;
    if (!processAxis(aPos.Y() - aCol.halfHeight, aPos.Y() + aCol.halfHeight, bPos.Y() - bCol.halfHeight, bPos.Y() + bCol.halfHeight, vy))
      return false;

    firstTimeOfCollision = tFirst;
    if (tFirst > tLast) return false;

    float xEntry = (vx != 0) ? ((vx > 0 ? (bPos.X() - bCol.halfWidth - (aPos.X() + aCol.halfWidth)) : (bPos.X() + bCol.halfWidth - (aPos.X() - aCol.halfWidth))) / vx) : -INFINITY;
    float yEntry = (vy != 0) ? ((vy > 0 ? (bPos.Y() - bCol.halfHeight - (aPos.Y() + aCol.halfHeight)) : (bPos.Y() + bCol.halfHeight - (aPos.Y() - aCol.halfHeight))) / vy) : -INFINITY;

    normal = (xEntry > yEntry) ? (vx > 0 ? Math::Vec2D{ -1, 0 } : Math::Vec2D{ 1, 0 })
      : (vy > 0 ? Math::Vec2D{ 0, -1 } : Math::Vec2D{ 0, 1 });

    return true;
  }
};
