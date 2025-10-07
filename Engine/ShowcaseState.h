//-----------------------------------------------------------------------------------
// file:    ShowcaseState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the showcase game state that demonstrates core engine
//          features including sprite animation, sound playback, collision events,
//          resource management, and ECS systems working together.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "StateManager.h"
#include "Core.h"
#include "Components.h"
#include "BasicComponents.h"
#include "RenderSystem.h"
//#include "BackgroundSystem.h"
#include "BasicMovementSystem.h"
#include "BasicEvents.h"
#include "InputManager.h"
#include "Resources.h"
#include "Precompiled.h"
#include "RotationSystem.h"

/* ===================== //
// Custom Events         //
// ===================== */
/*=================================================================================
Struct Brief:
Event dispatched when one entity collects another entity. Includes information
about the collector, the collected item, and points awarded.
==================================================================================*/
struct CollectEvent {
  ECS::EntityID collector;
  ECS::EntityID collected;
  s32 points;
};

/*=================================================================================
Struct Brief:
Event dispatched when one entity attacks another entity. Contains information
about the attacker, target, and damage dealt.
==================================================================================*/
struct AttackEvent {
  ECS::EntityID attacker;
  ECS::EntityID target;
  s32 damage;
};

class ShowcaseState : public Engine::GameState<ShowcaseState> {
private:
  // Entities
  ECS::Entity player;
  ECS::Entity enemy;
  ECS::Entity banana;
  ECS::Entity background_entity;

  // Resources - automatically managed via shared_ptr
  Engine::Graphics::Texture player_idle_texture;
  Engine::Graphics::Texture player_punch_texture;
  Engine::Graphics::Texture player_run_texture;
  Engine::Graphics::Texture banana_texture;
  Engine::Graphics::Texture enemy_texture;
  Engine::Graphics::Texture background_texture;

  Engine::Audio::Sound walk_sound;

  Engine::Graphics::Font title_font;
  Engine::Graphics::Font info_font;

  // Player state
  bool is_walking = false;
  bool is_punching = false;
  bool facing_right = true;
  s32 banana_collected = 0;
  f32 punch_timer = 0.0f;
  const f32 punch_duration = 0.6f; // 12 frames * 0.05s

  // Player dimensions
  const f32 player_idle_width = 64.0f;
  const f32 player_idle_height = 64.0f;
  const f32 player_punch_width = 96.0f;
  const f32 player_punch_height = 64.0f;

public:
  void load() {
    Engine::Resources::set_resource_path("Assets/");

    // Load sprite sheet textures with Nearest filter for pixel art
    player_idle_texture = Engine::Resources::load_texture("images/Bro_Idle_Spritesheet.png");
    player_punch_texture = Engine::Resources::load_texture("images/Bro_Punch_Spritesheet.png");
    player_run_texture = Engine::Resources::load_texture("images/Bro_Run_Spritesheet.png");

    // Load object textures
    banana_texture = Engine::Resources::load_texture("images/banana.png");
    enemy_texture = Engine::Resources::load_texture("images/LazyMonkey_Idle_Spritesheet.png");

    // Load background texture with Linear filter for smooth backgrounds
    background_texture = Engine::Resources::load_texture("images/sample_bg.png");

    // Load audio
    walk_sound = Engine::Resources::load_sound("sounds/move1.wav");

    // Load fonts
    title_font = Engine::Resources::load_font("fonts/PlayfairDisplay-VariableFont_wght.ttf", 82);
    info_font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 48);
  }

  void init() {
    // Create texture background
   /* background_entity = ECS::World::create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Background>()
      .add_component<Components::Texture>(background_texture);*/

      // Create enemy
    enemy = ECS::World::create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Transform2D>(800.0f, 655.0f, 0.0f, 64.0f * 2.0f, 64.0f * 2.0f)
      .add_component<Components::Texture>(enemy_texture)
      .add_component<Components::Health>(50, 50)
      .add_component<Components::Color>(255.0f, 255.0f, 255.0f, 255.0f)
      .add_component<Components::TextureAtlas>(64, 64, 1, 5, 5)
      .add_component<Components::Animation>(5, 0.15f);

    // Create banana
    banana = ECS::World::create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Transform2D>(1200.0f, 675.0f, 0.0f, 20.0f * 3.0f, 12.0f * 3.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(255.0f, 255.0f, 255.0f, 255.0f)
      .add_component<Components::RigidBody>();

    banana.get_component<Components::RigidBody>()->rotation = 0.0f;
    banana.get_component<Components::RigidBody>()->angular_velocity = 45.0f;

    // Create player with idle animation
    player = ECS::World::create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Transform2D>(200.0f, 655.0f, 0.0f, player_idle_width * 2.0f, player_idle_height * 2.0f)
      .add_component<Components::Texture>(player_idle_texture)
      .add_component<Components::RigidBody>()
      .add_component<Components::Health>(100, 100)
      .add_component<Components::Color>(255.0f, 255.0f, 255.0f, 255.0f)
      .add_component<Components::TextureAtlas>(64, 64, 1, 7, 7)
      .add_component<Components::Animation>(7, 0.15f);

    // Add systems
    //ECS::World::add_system<Systems::TextureBackgroundSystem>();
    ECS::World::add_system<Systems::AnimationUpdateSystem>();
    ECS::World::add_system<Systems::StaticSpriteRenderSystem>();
    ECS::World::add_system<Systems::AtlasSpriteRenderSystem>();
    ECS::World::add_system<BasicMovementSystem>();
    ECS::World::add_system<RotationSystem>();

    ECS::World::add_event_listener<AttackEvent>([this](const AttackEvent& event) {
      std::cout << "Attack event! Entity " << event.attacker
        << " attacks Entity " << event.target
        << " for " << event.damage << " damage!\n";

      // Play attack animation
      if (event.attacker == player.get_id()) {
        Engine::Audio::stop_sound(walk_sound);

        auto* player_texture = player.get_component<Components::Texture>();
        auto* player_transform = player.get_component<Components::Transform2D>();
        auto* player_animation = player.get_component<Components::Animation>();
        auto* player_atlas = player.get_component<Components::TextureAtlas>();
        if (player_texture && player_transform && player_animation) {
          player_texture->texture_handle = player_punch_texture;
          player_transform->scale.x = (facing_right ? 2.0f : -2.0f) * player_punch_width;
          player_transform->scale.y = 2.0f * player_punch_height;
          *player_animation = Components::Animation(12, 0.05f); // 12 frames, 0.05s each
          *player_atlas = Components::TextureAtlas(static_cast<s32>(player_punch_width), static_cast<s32>(player_punch_height), 1, 12, 12); // TODO: add a set_blah_blah

          std::cout << "Playing punch animation\n"; 
        }
      }

      // Apply damage to enemy
      auto target_entity = ECS::World::get_entity(event.target);
      if (target_entity.is_valid()) {
        std::cout << "Target entity is valid\n";

        if (target_entity.has_component<Components::Health>()) {
          std::cout << "Target has health component\n";
          auto* health = target_entity.get_component<Components::Health>();
          std::cout << "Health before damage: " << health->value << "\n";

          health->damage(event.damage);

          std::cout << "Health after damage: " << health->value << "\n";

          if (health->is_dead()) {
            std::cout << "Entity " << event.target << " has been defeated!\n";
            target_entity.destroy();
          }
        }
        else {
          std::cout << "ERROR: Target entity does not have health component!\n";
        }
      }
      else {
        std::cout << "ERROR: Target entity is not valid!\n";
      }
      });

    // Subscribe to collect events
    ECS::World::add_event_listener<CollectEvent>([this](const CollectEvent& event) {
      std::cout << "Collect event! Entity " << event.collector
        << " collected Entity " << event.collected
        << " for " << event.points << " points!\n";

      // Update score
      if (event.collector == player.get_id()) {
        banana_collected += event.points;
      }

      // Respawn the collected item
      auto collectedEntity = ECS::World::get_entity(event.collected);
      if (collectedEntity.is_valid()) {
        auto* transform = collectedEntity.get_component<Components::Transform2D>();
        if (transform) {
          transform->position.x = static_cast<f32>(rand() % 1400 + 100);
          transform->position.y = 675.0f; // Keep y position constant
        }
      }
      });
  }

  void update() {
    auto* player_transform = player.get_component<Components::Transform2D>();
    auto* rigid = player.get_component<Components::RigidBody>();
    auto* player_texture = player.get_component<Components::Texture>();

    if (!player_transform || !rigid || !player_texture) return;

    // Handle punch animation timing
    if (is_punching) {
      punch_timer += Engine::Time::get_fixed_dt();
      if (punch_timer >= punch_duration) {
        // Punch animation finished, return to idle or walk
        is_punching = false;
        punch_timer = 0.0f;

        // Return to appropriate animation state
        if (is_walking) {
          player_texture->texture_handle = player_run_texture;
          player_transform->scale.x = (facing_right ? 2.0f : -2.0f) * player_idle_width;
          player_transform->scale.y = 2.0f * player_idle_height;
          auto* player_animation = player.get_component<Components::Animation>();
          auto* player_atlas = player.get_component<Components::TextureAtlas>();
          if (player_animation && player_atlas) {
            *player_animation = Components::Animation(8, 0.1f);
            *player_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
          }
        }
        else {
          player_texture->texture_handle = player_idle_texture;
          player_transform->scale.x = (facing_right ? 2.0f : -2.0f) * player_idle_width;
          player_transform->scale.y = 2.0f * player_idle_height;
          auto* player_animation = player.get_component<Components::Animation>();
          auto* player_atlas = player.get_component<Components::TextureAtlas>();
          if (player_animation && player_atlas) {
            *player_animation = Components::Animation(7, 0.15f);
            *player_atlas = Components::TextureAtlas(64, 64, 1, 7, 7);
          }
        }
      }
    }

    // Only handle movement and animation changes if not punching
    if (!is_punching) {
      // Player movement input
      f32 speed = 300.0f;
      f32 new_vel_x = 0.0f;
      f32 new_vel_y = 0.0f;

      if (Engine::Input::is_key_held(GLFW_KEY_W)) {
        new_vel_y = -speed;
      }
      if (Engine::Input::is_key_held(GLFW_KEY_S)) {
        new_vel_y = speed;
      }
      if (Engine::Input::is_key_held(GLFW_KEY_A)) {
        new_vel_x = -speed;
      }
      if (Engine::Input::is_key_held(GLFW_KEY_D)) {
        new_vel_x = speed;
      }

      // Update facing direction based on horizontal movement
      if (new_vel_x > 0.0f) {
        facing_right = true;
      }
      else if (new_vel_x < 0.0f) {
        facing_right = false;
      }

      // Apply scale to flip sprite horizontally
      player_transform->scale.x = (facing_right ? 2.0f : -2.0f) * player_idle_width;
      player_transform->scale.y = 2.0f * player_idle_height;

      rigid->linear_velocity.x = new_vel_x;
      rigid->linear_velocity.y = new_vel_y;

      // Animation change based on movement
      bool shouldWalk = (new_vel_x != 0.0f || new_vel_y != 0.0f);

      if (shouldWalk && !is_walking) {
        // Switch to walking animation (8 frames)
        player_texture->texture_handle = player_run_texture;
        auto* player_animation = player.get_component<Components::Animation>();
        auto* player_atlas = player.get_component<Components::TextureAtlas>();
        if (player_animation && player_atlas) {
          *player_animation = Components::Animation(8, 0.1f);
          *player_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
        }
        Engine::Audio::play_sound(walk_sound, 1.0f, true);
        is_walking = true;
      }
      else if (!shouldWalk && is_walking) {
        // Switch to idle animation (7 frames)
        player_texture->texture_handle = player_idle_texture;
        auto* player_animation = player.get_component<Components::Animation>();
        auto* player_atlas = player.get_component<Components::TextureAtlas>();
        if (player_animation && player_atlas) {
          *player_animation = Components::Animation(7, 0.15f);
          *player_atlas = Components::TextureAtlas(64, 64, 1, 7, 7);
        }
        Engine::Audio::stop_sound(walk_sound);
        is_walking = false;
      }
    }

    // Attack enemy with E key (using event system)
    if (Engine::Input::is_key_pressed(GLFW_KEY_E) && !is_punching) {
      std::cout << "E key pressed! Player ID: " << player.get_id() << ", Enemy ID: " << enemy.get_id() << "\n";

      // Send attack event 
      ECS::World::send_event<AttackEvent>(player.get_id(), enemy.get_id(), 10);

      // Start punch animation
      is_punching = true;
      punch_timer = 0.0f;
    }

    // Collect banana with C key (using event system)
    if (Engine::Input::is_key_pressed(GLFW_KEY_C)) {
      ECS::World::send_event<CollectEvent>(player.get_id(), banana.get_id(), 1);
    }
  }

  void draw() {
    Engine::Graphics::clear_screen(background_texture);


    // Draw title text (first font)
    if (title_font && title_font->texture_atlas.id != 0) {
      Engine::Graphics::draw_text(
        title_font,
        "Engine Showcase",
        { 50.0f, 50.0f },
        1.0f,
        { 255.0f, 230.0f, 77.0f, 255.0f } // Gold color
      );
    }

    // Draw info text (second font)
    if (info_font && info_font->texture_atlas.id != 0) {
      Engine::Graphics::draw_text(
        info_font,
        "WASD: Move | E: Attack | C: Collect",
        { 50.0f, 140.0f },
        0.6f,
        { 255.0f, 255.0f, 255.0f, 255.0f } // White
      );

      Engine::Graphics::draw_text(
        info_font,
        "Bananas: " + std::to_string(banana_collected),
        { 50.0f, 200.0f },
        0.7f,
        { 255.0f, 255.0f, 0.0f, 255.0f } // Yellow
      );

      // Show enemy health
      if (enemy.is_valid() && enemy.has_component<Components::Health>()) {
        auto* health = enemy.get_component<Components::Health>();
        Engine::Graphics::draw_text(
          info_font,
          "Enemy HP: " + std::to_string(health->value) + "/" + std::to_string(health->max),
          { 50.0f, 240.0f },
          0.7f,
          { 255.0f, 0.0f, 0.0f, 255.0f } // Red
        );
      }
    }
  }

  void free() {
    // Stop any playing sounds
    if (is_walking) {
      Engine::Audio::stop_sound(walk_sound);
    }

    // Cleanup entities
    ECS::World::destroy_entities_with<Components::Tag<"ShowcaseState">>();

    // Remove systems
    //ECS::World::remove_system<TextureBackgroundSystem>();
    ECS::World::remove_system<Systems::StaticSpriteRenderSystem>();
    ECS::World::remove_system<BasicMovementSystem>();
    ECS::World::remove_system<RotationSystem>();
    //ECS::World::remove_system<RectRenderSystem>();

    // Remove event listeners
    ECS::World::remove_event_listener<AttackEvent>();
    ECS::World::remove_event_listener<CollectEvent>();
  }

  void unload() {
    // Resources are automatically cleaned up when shared_ptrs go out of scope
    // Reset them explicitly if desired (optional, happens automatically in destructor)
  }
};