//-----------------------------------------------------------------------------------
// file:    ShowcaseState.cpp
// project: Monkey Stoke My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of ShowcaseState demonstrating ECS, animation, audio,
//          and event systems working together.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ShowcaseState.h"

void ShowcaseState::load() {
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

void ShowcaseState::init() {
  // Create enemy
  enemy = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(800.0f, 655.0f, 0.0f, 64.0f * 2.0f, 64.0f * 2.0f)
    .add_component<Components::Texture>(enemy_texture)
    .add_component<Components::Health>(50, 50)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::TextureAtlas>(64, 64, 1, 5, 5)
    .add_component<Components::Animation>(5, 0.15f);

  // Create banana prefab
  Engine::ECS::PrefabManager::register_prefab("Banana", [this](Engine::ECS::Entity& entity) {
    entity.add_component<Components::Tag<"ShowcaseState">>()
      .add_component<Components::Transform2D>(1200.0f, 675.0f, 0.0f, 20.0f * 3.0f, 12.0f * 3.0f)
      .add_component<Components::Texture>(banana_texture)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::RigidBody>();

    auto* rigid = entity.get_component<Components::RigidBody>();
    rigid->rotation = 0.0f;
    rigid->angular_velocity = 45.0f;
    });

  // Create banana using prefab
  banana = Engine::ECS::PrefabManager::instantiate_prefab("Banana");

  // Create player with idle animation
  player = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(200.0f, 655.0f, 0.0f, player_idle_width * 2.0f, player_idle_height * 2.0f)
    .add_component<Components::Texture>(player_idle_texture)
    .add_component<Components::RigidBody>()
    .add_component<Components::Health>(100, 100)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::TextureAtlas>(64, 64, 1, 7, 7)
    .add_component<Components::Animation>(7, 0.15f);

  // Subscribe to attack events
  Engine::ECS::add_event_listener<AttackEvent>([this](const AttackEvent& event) {
    std::cout << "Attack event! Entity " << event.attacker
      << " attacks Entity " << event.target
      << " for " << event.damage << " damage!\n";

    // Play attack animation
    if (event.attacker == player.get_id()) {
      // Stop walk sound if playing
      if (walk_sound_channel != Engine::Audio::INVALID_CHANNEL) {
        Engine::Audio::stop_channel(walk_sound_channel);
        walk_sound_channel = Engine::Audio::INVALID_CHANNEL;
      }

      auto* player_texture = player.get_component<Components::Texture>();
      auto* player_transform = player.get_component<Components::Transform2D>();
      auto* player_animation = player.get_component<Components::Animation>();
      auto* player_atlas = player.get_component<Components::TextureAtlas>();
      if (player_texture && player_transform && player_animation) {
        player_texture->texture_handle = player_punch_texture;
        player_transform->scale.x = (facing_right ? 2.0f : -2.0f) * player_punch_width;
        player_transform->scale.y = 2.0f * player_punch_height;
        *player_animation = Components::Animation(12, 0.05f);
        *player_atlas = Components::TextureAtlas(static_cast<s32>(player_punch_width), static_cast<s32>(player_punch_height), 1, 12, 12);

        std::cout << "Playing punch animation\n";
      }
    }

    // Apply damage to enemy
    auto target_entity = Engine::ECS::get_entity(event.target);
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
  Engine::ECS::add_event_listener<CollectEvent>([this](const CollectEvent& event) {
    std::cout << "Collect event! Entity " << event.collector
      << " collected Entity " << event.collected
      << " for " << event.points << " points!\n";

    // Update score
    if (event.collector == player.get_id()) {
      banana_collected += event.points;
    }

    // Respawn the collected item
    auto collectedEntity = Engine::ECS::get_entity(event.collected);
    if (collectedEntity.is_valid()) {
      auto* transform = collectedEntity.get_component<Components::Transform2D>();
      if (transform) {
        transform->position.x = static_cast<f32>(rand() % 1400 + 100);
        transform->position.y = 675.0f;
      }
    }
    });
}

void ShowcaseState::update() {
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

      // Start playing walk sound on loop
      walk_sound_channel = Engine::Audio::play_sound(walk_sound, 1.0f, true);
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

      // Stop walk sound
      if (walk_sound_channel != Engine::Audio::INVALID_CHANNEL) {
        Engine::Audio::stop_channel(walk_sound_channel);
        walk_sound_channel = Engine::Audio::INVALID_CHANNEL;
      }
      is_walking = false;
    }
  }

  // Attack enemy with E key (using event system)
  if (Engine::Input::is_key_pressed(GLFW_KEY_E) && !is_punching) {
    std::cout << "E key pressed! Player ID: " << player.get_id() << ", Enemy ID: " << enemy.get_id() << "\n";

    // Send attack event 
    Engine::ECS::send_event<AttackEvent>(player.get_id(), enemy.get_id(), 10);

    // Start punch animation
    is_punching = true;
    punch_timer = 0.0f;
  }

  // Collect banana with C key (using event system)
  if (Engine::Input::is_key_pressed(GLFW_KEY_C)) {
    Engine::ECS::send_event<CollectEvent>(player.get_id(), banana.get_id(), 1);
  }
}

void ShowcaseState::draw() {
  Engine::Graphics::clear_screen(background_texture);

  // Draw title text (first font)
  if (title_font && title_font->texture_atlas.id != 0) {
    Engine::Graphics::draw_text(
      title_font,
      "Engine Showcase",
      { 50.0f, 50.0f },
      1.0f,
      { 1.0f, 0.9f, 0.3f, 1.0f } // Gold color
    );
  }

  // Draw info text (second font)
  if (info_font && info_font->texture_atlas.id != 0) {
    Engine::Graphics::draw_text(
      info_font,
      "WASD: Move | E: Attack | C: Collect",
      { 50.0f, 140.0f },
      0.6f,
      { 1.0f, 1.0f, 1.0f, 1.0f } // White
    );

    Engine::Graphics::draw_text(
      info_font,
      "Bananas: " + std::to_string(banana_collected),
      { 50.0f, 200.0f },
      0.7f,
      { 1.0f, 1.0f, 0.0f, 1.0f } // Yellow
    );

    // Show enemy health
    if (enemy.is_valid() && enemy.has_component<Components::Health>()) {
      auto* health = enemy.get_component<Components::Health>();
      Engine::Graphics::draw_text(
        info_font,
        "Enemy HP: " + std::to_string(health->value) + "/" + std::to_string(health->max),
        { 50.0f, 240.0f },
        0.7f,
        { 1.0f, 0.0f, 0.0f, 1.0f } // Red
      );
    }
  }
}

void ShowcaseState::free() {
  // Stop any playing sounds
  if (walk_sound_channel != Engine::Audio::INVALID_CHANNEL) {
    Engine::Audio::stop_channel(walk_sound_channel);
    walk_sound_channel = Engine::Audio::INVALID_CHANNEL;
  }

  // Cleanup entities
  Engine::ECS::destroy_entities_with<Components::Tag<"ShowcaseState">>();

  // Remove event listeners
  Engine::ECS::remove_event_listener<AttackEvent>();
  Engine::ECS::remove_event_listener<CollectEvent>();
}

void ShowcaseState::unload() {
  // Resources are automatically cleaned up when shared_ptrs go out of scope
  walk_sound.reset();

  // Clear unused resources from cache
  Engine::Resources::clear_unused_resources();
}