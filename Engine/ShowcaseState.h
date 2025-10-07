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
#include "BasicComponents.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include "BasicMovementSystem.h"
#include "BasicEvents.h"
#include "InputManager.h"
#include <GLFW/glfw3.h>

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
  // Prefabs
  s32 banana_prefab = -1;
  s32 lazy_monkey_prefab = -1;

  // Entities
  ECS::Entity player;
  ECS::Entity enemy;
  ECS::Entity banana;
  ECS::Entity background_entity;
  ECS::Entity texture_background;

  // Sprite Sheets
  s32 player_idle_sprite_sheet_id = -1;
  s32 player_punch_sprite_sheet_id = -1;
  s32 player_run_sprite_sheet_id = -1;

  // Audio
  s32 walkSoundId = -1;

  // Fonts
  s32 title_font_id = -1;
  s32 info_font_id = -1;

  // Texture Background
  s32 background_texture_id = -1;

  // Player state
  bool is_walking = false;
  bool facing_right = true;  // Add this to track facing direction
  s32 banana_collected = 0;

public:
  void load() {
    // Load prefabs
    banana_prefab = Engine::RESOURCES.load_object_prefab("banana", "Assets/objects/banana.json");
    lazy_monkey_prefab = Engine::RESOURCES.load_object_prefab("lazy_monkey", "Assets/objects/lazy_monkey.json");

    player_idle_sprite_sheet_id = Engine::RESOURCES.load_sprite_sheet(
      "Player_Idle",
      "Assets/images/Bro_Idle_Spritesheet.png",
      448 / 7,  // frame width
      64,       // frame height
      7,        // total frames
      8.0f,     // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    player_punch_sprite_sheet_id = Engine::RESOURCES.load_sprite_sheet(
      "Player_Punch",
      "Assets/images/Bro_Punch_Spritesheet.png",
      1152 / 12,  // frame width
      64,         // frame height
      12,         // total frames
      24.0f,      // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    player_run_sprite_sheet_id = Engine::RESOURCES.load_sprite_sheet(
      "Player_Run",
      "Assets/images/Bro_Run_Spritesheet.png",
      512 / 8,  // frame width
      64,       // frame height
      8,        // total frames
      16.0f,    // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    // Load audio
    walkSoundId = Engine::RESOURCES.load_sound("walk_sound", "Assets/sounds/move1.wav");

    // Load fonts
    title_font_id = Engine::RESOURCES.load_font("title_font", "Assets/fonts/PlayfairDisplay-VariableFont_wght.ttf", 82);
    info_font_id = Engine::RESOURCES.load_font("info_font", "Assets/fonts/Roboto-Regular.ttf", 48);

    // Load texture background
    background_texture_id = Engine::RESOURCES.load_texture(
      "showcase_bg",
      "Assets/images/sample_bg.png",
      Graphics::MinifyingFilter::Linear
    );
  }

  void init() {
    // Create solid color background
    background_entity = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Background>()
      .add_component<Components::StaticSprite>(background_texture_id);

    // Create enemy from prefab
    enemy = Engine::FACTORY.createObject(lazy_monkey_prefab)
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Position>(800.0f, 655.0f)
      .add_component<Components::Health>(50, 50)
      .add_component<Components::Scale>(2.0f, 2.0f);

    // Create banana from prefab
    banana = Engine::FACTORY.createObject(banana_prefab)
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Position>(1200.0f, 675.0f)
      .add_component<Components::Rotation>(0.0f)
      .add_component<Components::RotationSpeed>(90.0f)
      .add_component<Components::Scale>(3.0f, 3.0f);

    // Create player with idle animation
    player = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Position>(200.0f, 655.0f)
      .add_component<Components::Size>(64.0f, 64.0f)
      .add_component<Components::Velocity>(0.0f, 0.0f)
      .add_component<Components::AnimatedSprite>(player_idle_sprite_sheet_id)
      .add_component<Components::Health>(100, 100)
      .add_component<Components::Scale>(2.0f, 2.0f);

    // Add systems
    Engine::WORLD.add_system<TextureBackgroundSystem>();
    Engine::WORLD.add_system<StaticSpriteRenderSystem>();
    Engine::WORLD.add_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.add_system<BasicMovementSystem>();
    Engine::WORLD.add_system<RotationSystem>();
    Engine::WORLD.add_system<RectRenderSystem>();

    Engine::WORLD.add_event_listener<AttackEvent>([this](const AttackEvent& event) {
      std::cout << "Attack event! Entity " << event.attacker
        << " attacks Entity " << event.target
        << " for " << event.damage << " damage!\n";

      // Play attack animation
      if (event.attacker == player.get_id()) {
        Engine::AUDIO.stop_sound(walkSoundId); // Stop walking sound if playing

        auto* player_sprite = player.get_component<Components::AnimatedSprite>();
        if (player_sprite) {
          player_sprite->spriteSheetId = player_punch_sprite_sheet_id;
          player_sprite->reset();
          player.get_component<Components::Size>()->width = 96.0f; // Adjust size for punch sprites
          std::cout << "Playing punch animation\n";
        }
      }

      // Apply damage to enemy
      auto target_entity = Engine::WORLD.get_entity(event.target);
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
    Engine::WORLD.add_event_listener<CollectEvent>([this](const CollectEvent& event) {
      std::cout << "Collect event! Entity " << event.collector
        << " collected Entity " << event.collected
        << " for " << event.points << " points!\n";

      // Update score
      if (event.collector == player.get_id()) {
        banana_collected += event.points;
      }

      // Respawn the collected item
      auto collectedEntity = Engine::WORLD.get_entity(event.collected);
      if (collectedEntity.is_valid()) {
        auto* pos = collectedEntity.get_component<Components::Position>();
        if (pos) {
          pos->x = static_cast<f32>(rand() % 1400 + 100);
          675.0f; // Keep y position constant
        }
      }
      });
  }

  void update() {

    auto* player_pos = player.get_component<Components::Position>();
    auto* player_vel = player.get_component<Components::Velocity>();
    auto* player_sprite = player.get_component<Components::AnimatedSprite>();

    if (!player_pos || !player_vel || !player_sprite) return;

    // Check if punch animation has completed
    if (player_sprite->spriteSheetId == player_punch_sprite_sheet_id) {
      Graphics::SpriteSheetInfo info;
      if (Engine::GRAPHICS.get_sprite_sheet_info(player_punch_sprite_sheet_id, info)) {
        f32 totalDuration = info.total_frames / info.animation_speed;

        if (player_sprite->animationTime >= totalDuration) {
          // Punch animation completed, return to idle
          player_sprite->spriteSheetId = player_idle_sprite_sheet_id;
          player_sprite->reset();
          player.get_component<Components::Size>()->width = 64.0f; // Reset size after punch
          std::cout << "Punch animation completed, returning to idle\n";
        }
      }
    }

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
    auto* scale = player.get_component<Components::Scale>();
    if (scale) {
      scale->x = facing_right ? 2.0f : -2.0f;
      scale->y = 2.0f;
    }

    player_vel->x = new_vel_x;
    player_vel->y = new_vel_y;

    // Only change to walk/idle if not currently punching
    if (player_sprite->spriteSheetId != player_punch_sprite_sheet_id) {
      // Animation change based on movement
      bool shouldWalk = (new_vel_x != 0.0f || new_vel_y != 0.0f);

      if (shouldWalk && !is_walking) {
        // Switch to walking animation
        player_sprite->spriteSheetId = player_run_sprite_sheet_id;
        player_sprite->reset();
        Engine::AUDIO.play_sound_looped(walkSoundId);
        is_walking = true;
      }
      else if (!shouldWalk && is_walking) {
        // Switch to idle animation
        player_sprite->spriteSheetId = player_idle_sprite_sheet_id;
        player_sprite->reset();
        Engine::AUDIO.stop_sound(walkSoundId);
        is_walking = false;
      }
    }

    // Attack enemy with E key (using event system)
    if (Engine::Input::is_key_pressed(GLFW_KEY_E)) {
      std::cout << "E key pressed! Player ID: " << player.get_id() << ", Enemy ID: " << enemy.get_id() << "\n";

      // Send attack event 
      Engine::WORLD.send_event<AttackEvent>(player.get_id(), enemy.get_id(), 10);
    }

    // Collect banana with C key (using event system)
    if (Engine::Input::is_key_pressed(GLFW_KEY_C)) {
      Engine::WORLD.send_event<CollectEvent>(player.get_id(), banana.get_id(), 1);
    }
  }

  void draw() {
    // Draw title text (first font)
    if (title_font_id != -1) {
      Engine::GRAPHICS.draw_text(
        title_font_id,
        "Engine Showcase",
        50.0f, 50.0f,
        1.0f,
        { 1.0f, 0.9f, 0.3f, 1.0f } // Gold color
      );
    }

    // Draw info text (second font)
    if (info_font_id != -1) {
      Engine::GRAPHICS.draw_text(
        info_font_id,
        "WASD: Move | E: Attack | C: Collect",
        50.0f, 140.0f,
        0.6f,
        { 1.0f, 1.0f, 1.0f, 1.0f } // Light gray
      );

      Engine::GRAPHICS.draw_text(
        info_font_id,
        "Bananas: " + std::to_string(banana_collected),
        50.0f, 200.0f,
        0.7f,
        { 1.0f, 1.0f, 0.0f, 1.0f } // Yellow
      );

      // Show enemy health
      if (enemy.is_valid() && enemy.has_component<Components::Health>()) {
        auto* health = enemy.get_component<Components::Health>();
        Engine::GRAPHICS.draw_text(
          info_font_id,
          "Enemy HP: " + std::to_string(health->value) + "/" + std::to_string(health->max),
          50.0f, 240.0f,
          0.7f,
          { 1.0f, 0.0f, 0.0f, 1.0f } // Red
        );
      }
    }
  }

  void free() {
    // Stop any playing sounds
    if (is_walking) {
      Engine::AUDIO.stop_sound(walkSoundId);
    }

    // Cleanup entities
    Engine::WORLD.destroy_entities_with<Components::Tag<"ShowcaseState">>();

    // Remove systems
    Engine::WORLD.remove_system<TextureBackgroundSystem>();
    Engine::WORLD.remove_system<StaticSpriteRenderSystem>();
    Engine::WORLD.remove_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.remove_system<BasicMovementSystem>();
    Engine::WORLD.remove_system<RotationSystem>();
    Engine::WORLD.remove_system<RectRenderSystem>();

    // Remove event listeners
    Engine::WORLD.remove_event_listener<AttackEvent>();
    Engine::WORLD.remove_event_listener<CollectEvent>();
  }

  void unload() {
    // Unload prefabs
    Engine::RESOURCES.unload_object_prefab("lazy_monkey");
    Engine::RESOURCES.unload_object_prefab("banana");

    // Unload audio
    Engine::RESOURCES.unload_sound("walk_sound");

    // Unload fonts
    Engine::RESOURCES.unload_font("title_font");
    Engine::RESOURCES.unload_font("info_font");

    // Sprite sheets and textures are managed by ResourceManager
  }
};