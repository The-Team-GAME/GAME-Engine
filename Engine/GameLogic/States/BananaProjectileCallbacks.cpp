//-----------------------------------------------------------------------------------
// file:    BananaProjectileCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   Banana projectile state machine callbacks
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "./Components/HealthComponents.h"
#include "../../Engine/Engine/Systems/CollisionSystem.h"

namespace BananaProjectileCallbacks
{
  inline void spawn_banana_hit_vfx(Engine::ECS::Entity player_entity, const Math::Vec2D& pos, float x_offset)
  {
    auto prefab = Engine::Resources::load_prefab("prefabs/banana_hit_vfx.json");
    Engine::ECS::Entity vfx = Engine::ECS::instantiate_prefab(prefab);

    if (auto* transform = vfx.get_component<Components::Transform2D>())
    {
      transform->position = pos;
      transform->position.x += x_offset;
      transform->position.y -= 20.0f;
    }
    vfx.add_component<Components::Parent>(player_entity.get_id());
    vfx.add_component<Components::Lifetime>(0.7f);

  }

  // ========================================================================
  // Flying
  // ========================================================================

  inline void flying_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    Log::info("Banana Projectile: Entered Flying state");
  }

  inline void flying_on_update([[maybe_unused]] Engine::ECS::Entity entity)
  {
    
  }

  inline void flying_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
  }

  // ========================================================================
  // HitPlayer
  // ========================================================================

  inline void hit_player_on_enter(Engine::ECS::Entity entity)
  {
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (player_entity.is_valid())
    {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();
      auto* banana_rigid = entity.get_component<Components::RigidBody>();

      if (player_transform)
      {
        float x_offset = 0.0f;

        if (banana_rigid)
        {
          // Banana moving right means it came from left side
          if (banana_rigid->velocity.x > 0.0f)
            x_offset = -30.0f;
          else
            x_offset = 30.0f;
        }

        spawn_banana_hit_vfx(player_entity, player_transform->position, x_offset);
      }
    }

    if (auto* rigid = entity.get_component<Components::RigidBody>())
    {
      rigid->velocity = { 0.0f, 0.0f };
      rigid->acceleration = { 0.0f, 0.0f };
      rigid->angular_velocity = 0.0f;
    }

    if (entity.has_component<Components::Attack>())
    {
      entity.remove_component<Components::Attack>();
    }

    entity.remove_from_layer("damage_player");


    entity.add_component<Components::Lifetime>(0.5f);

    Log::info("Banana Projectile: Hit player");
  }

  inline void hit_player_on_update([[maybe_unused]] Engine::ECS::Entity entity)
  {
  }

  inline void hit_player_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
  }

} // namespace BananaProjectileCallbacks

REGISTER_FUNCTION(BananaProjectileCallbacks::flying_on_enter)
REGISTER_FUNCTION(BananaProjectileCallbacks::flying_on_update)
REGISTER_FUNCTION(BananaProjectileCallbacks::flying_on_exit)
REGISTER_FUNCTION(BananaProjectileCallbacks::hit_player_on_enter)
REGISTER_FUNCTION(BananaProjectileCallbacks::hit_player_on_update)
REGISTER_FUNCTION(BananaProjectileCallbacks::hit_player_on_exit)

REGISTER_STATE_MACHINE_BLUEPRINT(BananaProjectile, "Flying",
  STATE("Flying",
    BananaProjectileCallbacks::flying_on_enter,
    BananaProjectileCallbacks::flying_on_update,
    BananaProjectileCallbacks::flying_on_exit)
  STATE("HitPlayer",
    BananaProjectileCallbacks::hit_player_on_enter,
    BananaProjectileCallbacks::hit_player_on_update,
    BananaProjectileCallbacks::hit_player_on_exit)
)