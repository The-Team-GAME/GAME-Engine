//-----------------------------------------------------------------------------------
// file:    GravitySystem.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   This file defines the GravitySystem class, which applies gravitational
//          forces to entities with RigidBody and Gravity components.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include "../ECS/ECS.h"
#include "../Components/EngineComponents.h"

// For now this just clears into a solid color, but could be extended to do more complex background rendering
class GravitySystem : public ECS::System {
public:
  GravitySystem() {
    require_components<Components::RigidBody, Components::Gravity, Components::Collider>();
  }

  void update_entity(ECS::Entity& entity) override {
    auto* rb = entity.get_component<Components::RigidBody>();
    auto* g = entity.get_component<Components::Gravity>();
    auto* col = entity.get_component<Components::Collider>();

    if (!rb || !g) return;
		if (col->isStatic) return; // Static bodies don't move

    f32 dt = Engine::Time::get_fixed_dt();

    // Apply downward acceleration
    if (rb->isGrounded) return; // No gravity if grounded
    rb->acceleration.y += g->strength * 12000 * dt;
  }
};