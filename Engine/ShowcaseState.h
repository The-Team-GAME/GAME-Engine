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
  s32 bananaPrefab = -1;
  s32 lazyMonkeyPrefab = -1;

  // Entities
  ECS::Entity player;
  ECS::Entity enemy;
  ECS::Entity banana;
  ECS::Entity backgroundEntity;
  ECS::Entity textureBackground;

  // Sprite Sheets
  s32 playerIdleSpriteSheetId = -1;
  s32 playerPunchSpriteSheetId = -1;
  s32 playerRunSpriteSheetId = -1;

  // Audio
  s32 walkSoundId = -1;

  // Fonts
  s32 titleFontId = -1;
  s32 infoFontId = -1;

  // Texture Background
  s32 backgroundTextureId = -1;

  // Player state
  bool isWalking = false;
  bool facingRight = true;  // Add this to track facing direction
  s32 bananaCollected = 0;

public:
  void Load() {
    // Load prefabs
    bananaPrefab = Engine::RESOURCES.load_object_prefab("banana", "Assets/objects/banana.json");
    lazyMonkeyPrefab = Engine::RESOURCES.load_object_prefab("lazy_monkey", "Assets/objects/lazy_monkey.json");

    playerIdleSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
      "Player_Idle",
      "Assets/images/Bro_Idle_Spritesheet.png",
      448 / 7,  // frame width
      64,       // frame height
      7,        // total frames
      8.0f,     // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    playerPunchSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
      "Player_Punch",
      "Assets/images/Bro_Punch_Spritesheet.png",
      1152 / 12,  // frame width
      64,         // frame height
      12,         // total frames
      24.0f,      // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    playerRunSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
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
    titleFontId = Engine::RESOURCES.load_font("title_font", "Assets/fonts/PlayfairDisplay-VariableFont_wght.ttf", 82);
    infoFontId = Engine::RESOURCES.load_font("info_font", "Assets/fonts/Roboto-Regular.ttf", 48);

    // Load texture background
    backgroundTextureId = Engine::RESOURCES.load_texture(
      "showcase_bg",
      "Assets/images/sample_bg.png",
      Graphics::MinifyingFilter::Linear
    );
  }

  void Init() {
    // Create solid color background
    backgroundEntity = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.15f, 0.20f, 0.25f, 1.0f);

    // Create texture background (behind everything)
    textureBackground = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Position>(800.0f, 450.0f)
      .add_component<Components::Size>(1600.0f, 900.0f)
      .add_component<Components::StaticSprite>(backgroundTextureId)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.5f); // Semi-transparent


    // Create enemy from prefab
    enemy = Engine::FACTORY.createObject(lazyMonkeyPrefab)
      .add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Position>(800.0f, 655.0f)
      .add_component<Components::Health>(50, 50)
      .add_component<Components::Scale>(2.0f, 2.0f);

    // Create banana from prefab
    banana = Engine::FACTORY.createObject(bananaPrefab)
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
      .add_component<Components::AnimatedSprite>(playerIdleSpriteSheetId)
      .add_component<Components::Health>(100, 100)
      .add_component<Components::Scale>(2.0f, 2.0f);

    // Add systems
    Engine::WORLD.add_system<BackgroundSystem>();
    Engine::WORLD.add_system<StaticSpriteRenderSystem>();
    Engine::WORLD.add_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.add_system<BasicMovementSystem>();
    Engine::WORLD.add_system<RotationSystem>();

    Engine::WORLD.add_event_listener<AttackEvent>([this](const AttackEvent& event) {
      std::cout << "Attack event! Entity " << event.attacker
        << " attacks Entity " << event.target
        << " for " << event.damage << " damage!\n";

      // Play attack animation
      if (event.attacker == player.getId()) {
        Engine::AUDIO.stopSound(walkSoundId); // Stop walking sound if playing

        auto* playerSprite = player.get_component<Components::AnimatedSprite>();
        if (playerSprite) {
          playerSprite->spriteSheetId = playerPunchSpriteSheetId;
          playerSprite->reset();
          player.get_component<Components::Size>()->width = 96.0f; // Adjust size for punch sprites
          std::cout << "Playing punch animation\n";
        }
      }

      // Apply damage to enemy
      auto target_entity = Engine::WORLD.get_entity(event.target);
      if (target_entity.isValid()) {
        std::cout << "Target entity is valid\n";

        if (target_entity.has_component<Components::Health>()) {
          std::cout << "Target has health component\n";
          auto* health = target_entity.get_component<Components::Health>();
          std::cout << "Health before damage: " << health->value << "\n";

          health->damage(event.damage);

          std::cout << "Health after damage: " << health->value << "\n";

          if (health->isDead()) {
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
      if (event.collector == player.getId()) {
        bananaCollected += event.points;
      }

      // Respawn the collected item
      auto collectedEntity = Engine::WORLD.get_entity(event.collected);
      if (collectedEntity.isValid()) {
        auto* pos = collectedEntity.get_component<Components::Position>();
        if (pos) {
          pos->X() = static_cast<f32>(rand() % 1400 + 100);
          675.0f; // Keep y position constant
        }
      }
      });
  }

  void Update() {

    auto* playerPos = player.get_component<Components::Position>();
    auto* playerVel = player.get_component<Components::Velocity>();
    auto* playerSprite = player.get_component<Components::AnimatedSprite>();

    if (!playerPos || !playerVel || !playerSprite) return;

    // Check if punch animation has completed
    if (playerSprite->spriteSheetId == playerPunchSpriteSheetId) {
      Graphics::SpriteSheetInfo info;
      if (Engine::GRAPHICS.getSpriteSheetInfo(playerPunchSpriteSheetId, info)) {
        f32 totalDuration = info.totalFrames / info.animationSpeed;

        if (playerSprite->animationTime >= totalDuration) {
          // Punch animation completed, return to idle
          playerSprite->spriteSheetId = playerIdleSpriteSheetId;
          playerSprite->reset();
          player.get_component<Components::Size>()->width = 64.0f; // Reset size after punch
          std::cout << "Punch animation completed, returning to idle\n";
        }
      }
    }

    // Player movement input
    f32 speed = 300.0f;
    f32 newVelX = 0.0f;
    f32 newVelY = 0.0f;

    if (Engine::Input::IsKeyHeld(GLFW_KEY_W)) {
      newVelY = -speed;
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_S)) {
      newVelY = speed;
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_A)) {
      newVelX = -speed;
    }
    if (Engine::Input::IsKeyHeld(GLFW_KEY_D)) {
      newVelX = speed;
    }

    // Update facing direction based on horizontal movement
    if (newVelX > 0.0f) {
      facingRight = true;
    }
    else if (newVelX < 0.0f) {
      facingRight = false;
    }

    // Apply scale to flip sprite horizontally
    auto* scale = player.get_component<Components::Scale>();
    if (scale) {
      scale->X() = facingRight ? 2.0f : -2.0f;
      scale->Y() = 2.0f;
    }

    playerVel->X() = newVelX;
    playerVel->Y() = newVelY;

    // Only change to walk/idle if not currently punching
    if (playerSprite->spriteSheetId != playerPunchSpriteSheetId) {
      // Animation change based on movement
      bool shouldWalk = (newVelX != 0.0f || newVelY != 0.0f);

      if (shouldWalk && !isWalking) {
        // Switch to walking animation
        playerSprite->spriteSheetId = playerRunSpriteSheetId;
        playerSprite->reset();
        Engine::AUDIO.playSoundLooped(walkSoundId);
        isWalking = true;
      }
      else if (!shouldWalk && isWalking) {
        // Switch to idle animation
        playerSprite->spriteSheetId = playerIdleSpriteSheetId;
        playerSprite->reset();
        Engine::AUDIO.stopSound(walkSoundId);
        isWalking = false;
      }
    }

    // Attack enemy with E key (using event system)
    if (Engine::Input::IsKeyPressed(GLFW_KEY_E)) {
      std::cout << "E key pressed! Player ID: " << player.getId() << ", Enemy ID: " << enemy.getId() << "\n";

      // Send attack event 
      Engine::WORLD.send_event<AttackEvent>(player.getId(), enemy.getId(), 10);
    }

    // Collect banana with C key (using event system)
    if (Engine::Input::IsKeyPressed(GLFW_KEY_C)) {
      Engine::WORLD.send_event<CollectEvent>(player.getId(), banana.getId(), 1);
    }
  }

  void Draw() {
    // Draw title text (first font)
    if (titleFontId != -1) {
      Engine::GRAPHICS.draw_text(
        titleFontId,
        "Engine Showcase",
        50.0f, 50.0f,
        1.0f,
        { 1.0f, 0.9f, 0.3f, 1.0f } // Gold color
      );
    }

    // Draw info text (second font)
    if (infoFontId != -1) {
      Engine::GRAPHICS.draw_text(
        infoFontId,
        "WASD: Move | E: Attack | C: Collect",
        50.0f, 140.0f,
        0.6f,
        { 1.0f, 1.0f, 1.0f, 1.0f } // Light gray
      );

      Engine::GRAPHICS.draw_text(
        infoFontId,
        "Bananas: " + std::to_string(bananaCollected),
        50.0f, 200.0f,
        0.7f,
        { 1.0f, 1.0f, 0.0f, 1.0f } // Yellow
      );

      // Show enemy health
      if (enemy.isValid() && enemy.has_component<Components::Health>()) {
        auto* health = enemy.get_component<Components::Health>();
        Engine::GRAPHICS.draw_text(
          infoFontId,
          "Enemy HP: " + std::to_string(health->value) + "/" + std::to_string(health->max),
          50.0f, 240.0f,
          0.7f,
          { 1.0f, 0.0f, 0.0f, 1.0f } // Red
        );
      }
    }
  }

  void Free() {
    // Stop any playing sounds
    if (isWalking) {
      Engine::AUDIO.stopSound(walkSoundId);
    }

    // Cleanup entities
    Engine::WORLD.destroy_entities_with<Components::Tag<"ShowcaseState">>();

    // Remove systems
    Engine::WORLD.remove_system<BackgroundSystem>();
    Engine::WORLD.remove_system<StaticSpriteRenderSystem>();
    Engine::WORLD.remove_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.remove_system<BasicMovementSystem>();
    Engine::WORLD.remove_system<RotationSystem>();
  }

  void Unload() {
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