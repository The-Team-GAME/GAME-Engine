#pragma once

#include "StateManager.h"
#include "Core.h"
#include "BasicComponents.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include "MovementSystem.h"
#include "BasicEvents.h"
#include "InputManager.h"
#include <GLFW/glfw3.h>

class ShowcaseState : public Engine::GameState<ShowcaseState> {
private:
  // Prefabs
  s32 enemyPrefab = -1;
  s32 coinPrefab = -1;

  // Entities
  ECS::Entity player;
  ECS::Entity enemy;
  ECS::Entity coin;
  ECS::Entity backgroundEntity;
  ECS::Entity textureBackground;

  // Sprite Sheets
  s32 idleSpriteSheetId = -1;
  s32 walkSpriteSheetId = -1;

  // Audio
  s32 jumpSoundId = -1;
  s32 collectSoundId = -1;
  s32 bgMusicId = -1;

  // Fonts
  s32 titleFontId = -1;
  s32 infoFontId = -1;

  // Texture Background
  s32 backgroundTextureId = -1;

  // Player state
  bool isWalking = false;
  s32 coinsCollected = 0;

public:
  void Load() {
    // Load prefabs
    enemyPrefab = Engine::RESOURCES.load_object_prefab("showcase_enemy", "Assets/objects/enemy.json");
    coinPrefab = Engine::RESOURCES.load_object_prefab("showcase_coin", "Assets/objects/banana.json");

    // Load sprite sheets for player animation
    idleSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
      "Player_Idle",
      "Assets/images/LazyMonkey_Idle_Spritesheet.png",
      320 / 5,  // frame width
      64,       // frame height
      5,        // total frames
      8.0f,     // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    walkSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
      "Player_Walk",
      "Assets/images/LazyMonkey_Walk_Spritesheet.png",
      384 / 6,  // frame width
      64,       // frame height
      6,        // total frames
      12.0f,    // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    // Load audio
    jumpSoundId = Engine::RESOURCES.load_sound("jump_sound", "Assets/sounds/move1.wav");
    collectSoundId = Engine::RESOURCES.load_sound("collect_sound", "Assets/sounds/move1.wav");
    bgMusicId = Engine::RESOURCES.load_sound("bg_music", "Assets/sounds/move1.wav");

    // Load fonts
    titleFontId = Engine::RESOURCES.load_font("title_font", "Assets/fonts/PlayfairDisplay-VariableFont_wght.ttf", 48);
    infoFontId = Engine::RESOURCES.load_font("info_font", "Assets/fonts/Roboto-Regular.ttf", 24);

    // Load texture background
    backgroundTextureId = Engine::RESOURCES.load_texture(
      "showcase_bg",
      "Assets/images/sample_bg.png",
      Graphics::MinifyingFilter::Linear
    );
  }

  void Init() {
    // Create solid color background
    backgroundEntity = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"ShowcaseState">>()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.15f, 0.20f, 0.25f, 1.0f);

    // Create texture background (behind everything)
    textureBackground = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"ShowcaseState">>()
      .addComponent<Components::Position>(800.0f, 450.0f)
      .addComponent<Components::Size>(1600.0f, 900.0f)
      .addComponent<Components::StaticSprite>(backgroundTextureId)
      .addComponent<Components::Color>(1.0f, 1.0f, 1.0f, 0.5f); // Semi-transparent

    // Create player with idle animation
    player = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"ShowcaseState">>()
      .addComponent<Components::Position>(200.0f, 450.0f)
      .addComponent<Components::Size>(128.0f, 128.0f)
      .addComponent<Components::Velocity>(0.0f, 0.0f)
      .addComponent<Components::AnimatedSprite>(idleSpriteSheetId)
      .addComponent<Components::Health>(100, 100);

    // Create enemy from prefab
    enemy = Engine::FACTORY.createObject(enemyPrefab)
      .addComponent<Components::Tag<"ShowcaseState">>()
      .addComponent<Components::Position>(800.0f, 450.0f)
      .addComponent<Components::Rotation>(0.0f)
      .addComponent<Components::RotationSpeed>(45.0f)
      .addComponent<Components::Health>(50, 50);

    // Create coin from prefab
    coin = Engine::FACTORY.createObject(coinPrefab)
      .addComponent<Components::Tag<"ShowcaseState">>()
      .addComponent<Components::Position>(1200.0f, 450.0f)
      .addComponent<Components::Rotation>(0.0f)
      .addComponent<Components::RotationSpeed>(90.0f);

    // Add systems
    Engine::WORLD.addSystem<BackgroundSystem>();
    Engine::WORLD.addSystem<StaticSpriteRenderSystem>();
    Engine::WORLD.addSystem<AnimatedSpriteRenderSystem>();
    Engine::WORLD.addSystem<MovementSystem>();
    Engine::WORLD.addSystem<RotationSystem>();

    // Play background music (if your audio system supports it)
    if (bgMusicId != -1) {
      Engine::AUDIO.playSound(bgMusicId);
    }

    // Subscribe to damage events
    Engine::WORLD.addEventListener<DamageEvent>([this](const DamageEvent& event) {
      std::cout << "Damage event received! Entity " << event.target 
                << " took " << event.amount << " damage!\n";
    });
  }

  void Update() {
    auto* playerPos = player.getComponent<Components::Position>();
    auto* playerVel = player.getComponent<Components::Velocity>();
    auto* playerSprite = player.getComponent<Components::AnimatedSprite>();

    if (!playerPos || !playerVel || !playerSprite) return;

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

    playerVel->X() = newVelX;
    playerVel->Y() = newVelY;

    // Animation change based on movement
    bool shouldWalk = (newVelX != 0.0f || newVelY != 0.0f);
    
    if (shouldWalk && !isWalking) {
      // Switch to walking animation
      playerSprite->spriteSheetId = walkSpriteSheetId;
      playerSprite->reset();
      isWalking = true;
    }
    else if (!shouldWalk && isWalking) {
      // Switch to idle animation
      playerSprite->spriteSheetId = idleSpriteSheetId;
      playerSprite->reset();
      isWalking = false;
    }

    // Jump sound on spacebar
    if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE)) {
      if (jumpSoundId != -1) {
        Engine::AUDIO.playSound(jumpSoundId);
      }
      std::cout << "Jump!\n";
    }

    // Attack enemy with E key (demonstrate messaging system)
    if (Engine::Input::IsKeyPressed(GLFW_KEY_E)) {
      if (enemy.isValid() && enemy.hasComponent<Components::Health>()) {
        Engine::WORLD.sendEvent<DamageEvent>(enemy.getId(), 10);
        
        auto* enemyHealth = enemy.getComponent<Components::Health>();
        if (enemyHealth->isDead()) {
          std::cout << "Enemy defeated!\n";
          enemy.destroy();
        }
      }
    }

    // Collect coin with C key
    if (Engine::Input::IsKeyPressed(GLFW_KEY_C)) {
      if (coin.isValid()) {
        if (collectSoundId != -1) {
          Engine::AUDIO.playSound(collectSoundId);
        }
        coinsCollected++;
        std::cout << "Coin collected! Total: " << coinsCollected << "\n";
        
        // Respawn coin at random position
        auto* coinPos = coin.getComponent<Components::Position>();
        if (coinPos) {
          coinPos->X() = static_cast<f32>(rand() % 1400 + 100);
          coinPos->Y() = static_cast<f32>(rand() % 700 + 100);
        }
      }
    }
  }

  void Draw() {
    // Draw title text (first font)
    if (titleFontId != -1) {
      Engine::GRAPHICS.drawText(
        titleFontId,
        "Engine Showcase",
        50.0f, 50.0f,
        1.0f,
        { 1.0f, 0.9f, 0.3f, 1.0f } // Gold color
      );
    }

    // Draw info text (second font)
    if (infoFontId != -1) {
      Engine::GRAPHICS.drawText(
        infoFontId,
        "WASD: Move | SPACE: Jump | E: Attack | C: Collect",
        50.0f, 120.0f,
        0.6f,
        { 0.8f, 0.8f, 0.8f, 1.0f } // Light gray
      );

      Engine::GRAPHICS.drawText(
        infoFontId,
        "Coins: " + std::to_string(coinsCollected),
        50.0f, 160.0f,
        0.7f,
        { 1.0f, 1.0f, 0.0f, 1.0f } // Yellow
      );

      // Show player health
      if (player.isValid() && player.hasComponent<Components::Health>()) {
        auto* health = player.getComponent<Components::Health>();
        Engine::GRAPHICS.drawText(
          infoFontId,
          "Player HP: " + std::to_string(health->value) + "/" + std::to_string(health->max),
          50.0f, 200.0f,
          0.7f,
          { 0.0f, 1.0f, 0.0f, 1.0f } // Green
        );
      }

      // Show enemy health
      if (enemy.isValid() && enemy.hasComponent<Components::Health>()) {
        auto* health = enemy.getComponent<Components::Health>();
        Engine::GRAPHICS.drawText(
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
    // Cleanup entities
    Engine::WORLD.destroyEntitiesWith<Components::Tag<"ShowcaseState">>();

    // Remove systems
    Engine::WORLD.removeSystem<BackgroundSystem>();
    Engine::WORLD.removeSystem<StaticSpriteRenderSystem>();
    Engine::WORLD.removeSystem<AnimatedSpriteRenderSystem>();
    Engine::WORLD.removeSystem<MovementSystem>();
    Engine::WORLD.removeSystem<RotationSystem>();
  }

  void Unload() {
    // Unload prefabs
    Engine::RESOURCES.unload_object_prefab("showcase_enemy");
    Engine::RESOURCES.unload_object_prefab("showcase_coin");

    // Unload audio
    Engine::RESOURCES.unload_sound("jump_sound");
    Engine::RESOURCES.unload_sound("collect_sound");
    Engine::RESOURCES.unload_sound("bg_music");

    // Unload fonts
    Engine::RESOURCES.unload_font("title_font");
    Engine::RESOURCES.unload_font("info_font");

    // Sprite sheets and textures are managed by ResourceManager
  }
};