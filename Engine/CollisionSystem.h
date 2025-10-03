//-----------------------------------------------------------------------------------
// file:    CollisionSystem.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   This file defines the CollisionSystem class, which handles collision
//          detection and resolution for entities in the ECS framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once


#include "ECS/System.h" 
#include "BasicComponents.h"
#include "TypeDef.h"
#include <iostream>
#include <algorithm>

class CollisionSystem : public ECS::System {
public:
  CollisionSystem() {
    require_components<Components::Collider, Components::Position, Components::RigidBody>();
  }

  void update_entity(ECS::Entity& entity) override {
    auto* posA = entity.get_component<Components::Position>();
    auto* colA = entity.get_component<Components::Collider>();
    auto* rbA = entity.get_component<Components::RigidBody>();

    if (!posA || !colA || !rbA) return;

    const f32 dt = Engine::Time::get_fixed_dt();
    f32 foot_offset = 0.01f;
    Math::Vec2D foot_pos = { posA->X(), posA->Y() - colA->halfHeight - foot_offset };

    for (ECS::Entity other_entity : world->get_entities_with<Components::Collider>()) {
      auto other_id = other_entity.get_id();
      if (other_id <= entity.get_id()) continue; // Skip self + duplicates

      auto* posB = world->get_component<Components::Position>(other_id);
      auto* colB = world->get_component<Components::Collider>(other_id);
      auto* rbB = world->get_component<Components::RigidBody>(other_id); // nullptr if static

      if (!posB || !colB) continue;

      bool handled_dynamic = false;

      // --- Dynamic vs Dynamic collision ---
      if (!colA->isStatic && !colB->isStatic && rbA && rbB) {
        f32 t_first = 0.0f;
        Math::Vec2D normal;
        if (swept_aabb_dynamic(*posA, *rbA, *colA, *posB, rbB->velocity, *colB, dt, t_first, normal)) {
          if (t_first <= dt) {
            colA->dynamicCollisionTime = t_first;
            colA->dynamicNormal = normal;

            colB->dynamicCollisionTime = t_first;
            colB->dynamicNormal = -normal;

            /*std::cout << "Dynamic collision between " << entity.get_id() << " and " << otherEntity.get_id()
              << " normal=(" << normal.x << ", " << normal.y << ")\n";*/

            handled_dynamic = true;
          }
        }
      }

      // --- Static collision fallback ---
      if (!handled_dynamic) {
        Math::Vec2D overlap;
        if (check_aabb_collision(*posA, *colA, *posB, *colB, overlap)) {
          store_penetration(*colA, *colB, overlap);
        }
      }
    }
  }

private:
  bool check_aabb_collision(const Components::Position& a_pos, const Components::Collider& a_col,
    const Components::Position& b_pos, const Components::Collider& b_col,
    Math::Vec2D& out_overlap)
  {
    f32 dx = b_pos.X() - a_pos.X();
    f32 px = (a_col.halfWidth + b_col.halfWidth) - std::abs(dx);
    if (px <= 0) return false;

    f32 dy = b_pos.Y() - a_pos.Y();
    f32 py = (a_col.halfHeight + b_col.halfHeight) - std::abs(dy);
    if (py <= 0) return false;

    if (px < py) out_overlap = { (dx > 0) ? px : -px, 0.0f };
    else         out_overlap = { 0.0f, (dy > 0) ? py : -py };

    return true;
  }

  void store_penetration(Components::Collider& a_col, Components::Collider& b_col, const Math::Vec2D& overlap) {
    if (a_col.isStatic) b_col.penetration = overlap;
    if (b_col.isStatic) a_col.penetration = overlap * -1;

    if (!a_col.isStatic && !b_col.isStatic) {
      a_col.penetration = overlap * -1;
      b_col.penetration = overlap;
    }
  }

  bool swept_aabb_dynamic(const Components::Position& a_pos, const Components::RigidBody& a_rb, const Components::Collider& a_col,
    const Components::Position& b_pos, const Math::Vec2D& b_vel, const Components::Collider& b_col,
    f32 dt, f32& first_time_of_collision, Math::Vec2D& normal)
  {
    f32 vx = a_rb.velocity.X() - b_vel.X();
    f32 vy = a_rb.velocity.Y() - b_vel.Y();

    f32 t_first = 0.0f;
    f32 t_last = dt;

    auto processAxis = [&](f32 a_min, f32 a_max, f32 b_min, f32 b_max, f32 v) -> bool {
      if (v < 0.0f) {
        if (a_max < b_min) t_first = std::max((a_max - b_min) / v, t_first);
        if (a_min < b_max) t_last = std::min((a_min - b_max) / v, t_last);
        if (a_min > b_max) return false;
      }
      else if (v > 0.0f) {
        if (a_min > b_max) t_first = std::max((a_min - b_max) / v, t_first);
        if (a_max > b_min) t_last = std::min((a_max - b_min) / v, t_last);
        if (a_max < b_min) return false;
      }
      else {
        if (a_max < b_min || a_min > b_max) return false;
      }
      return true;
      };

    if (!processAxis(a_pos.X() - a_col.halfWidth, a_pos.X() + a_col.halfWidth, b_pos.X() - b_col.halfWidth, b_pos.X() + b_col.halfWidth, vx))
      return false;
    if (!processAxis(a_pos.Y() - a_col.halfHeight, a_pos.Y() + a_col.halfHeight, b_pos.Y() - b_col.halfHeight, b_pos.Y() + b_col.halfHeight, vy))
      return false;

    first_time_of_collision = t_first;
    if (t_first > t_last) return false;

    f32 x_entry = (vx != 0) ? ((vx > 0 ? (b_pos.X() - b_col.halfWidth - (a_pos.X() + a_col.halfWidth)) : (b_pos.X() + b_col.halfWidth - (a_pos.X() - a_col.halfWidth))) / vx) : -INFINITY;
    f32 y_entry = (vy != 0) ? ((vy > 0 ? (b_pos.Y() - b_col.halfHeight - (a_pos.Y() + a_col.halfHeight)) : (b_pos.Y() + b_col.halfHeight - (a_pos.Y() - a_col.halfHeight))) / vy) : -INFINITY;

    normal = (x_entry > y_entry) ? (vx > 0 ? Math::Vec2D{ -1, 0 } : Math::Vec2D{ 1, 0 })
      : (vy > 0 ? Math::Vec2D{ 0, -1 } : Math::Vec2D{ 0, 1 });

    return true;
  }
};
