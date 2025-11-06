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
#include "../Components/HealthComponents.h"

void ShowcaseState::load() {
  Engine::Resources::set_resource_path("Assets/");


  background_texture = Engine::Resources::load_texture("images/sample_bg.png");

  // Load audio
  walk_sound = Engine::Resources::load_sound("sounds/move1.wav");
  jump_sound = Engine::Resources::load_sound("sounds/jump.wav");
  punch_sound = Engine::Resources::load_sound("sounds/Punch.wav");
  bgm_sound = Engine::Resources::load_sound("sounds/A STORY TO TELL - Jazz MSCLJZ2_26.wav");

  // Load fonts
  title_font = Engine::Resources::load_font("fonts/PlayfairDisplay-VariableFont_wght.ttf", 82);
  info_font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 48);


}

void ShowcaseState::init() {
  camera = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Camera>("Main Camera", true);

  // Create enemy
  enemy = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(800.0f, 655.0f, 0.0f, 64.0f * 2.0f, 64.0f * 2.0f)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"))
    .add_component<Components::Health>(50, 50)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::TextureAtlas>(64, 64, 1, 5, 5)
    .add_component<Components::Animation>(5, 0.15f);

  // Create banana
  banana = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(1200.0f, 675.0f, 0.0f, 20.0f * 3.0f, 12.0f * 3.0f)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/banana.png"))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::RigidBody>(false, false);

  auto* rigid = banana.get_component<Components::RigidBody>();
  rigid->rotation = 0.0f;
  rigid->angular_velocity = 45.0f;

  // Create AnimationState for player with all three animation states
  Components::AnimationState player_anim_state;

  // Add idle state (24 frames) - looping, no auto-transition
  player_anim_state.add_state(
    IDLE_STATE,
    Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png")),
    Components::TextureAtlas(64, 64, 1, 24, 24),
    Components::Animation(24, 0.1f),
    Math::Vec2D(player_idle_width * 2.0f, player_idle_height * 2.0f),
    true,  // is_looping
    std::nullopt  // no next_state (looping animations don't auto-transition)
  );

  // Add run state (8 frames) - looping, no auto-transition
  player_anim_state.add_state(
    RUN_STATE,
    Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png")),
    Components::TextureAtlas(64, 64, 1, 8, 8),
    Components::Animation(8, 0.1f),
    Math::Vec2D(player_idle_width * 2.0f, player_idle_height * 2.0f),
    true,  // is_looping
    std::nullopt  // no next_state
  );

  // Add punch state (6 frames) - one-shot, auto-transitions to idle
  player_anim_state.add_state(
    PUNCH_STATE,
    Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_threehit_spritesheet.png")),
    Components::TextureAtlas(96, 64, 1, 19, 6),
    Components::Animation(6, 0.05f),
    Math::Vec2D(player_punch_width * 2.0f, player_punch_height * 2.0f),
    false,  // is_looping = false (one-shot animation)
    IDLE_STATE  // next_state = auto-transition to idle when done
  );

  // Set initial state to idle
  player_anim_state.set_entity_state(IDLE_STATE);

  // Create player with AnimationState
  player = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(200.0f, 655.0f, 0.0f, player_idle_width * 2.0f, player_idle_height * 2.0f)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"))
    .add_component<Components::TextureAtlas>(64, 64, 1, 24, 24)
    .add_component<Components::RigidBody>(false, true)
    .add_component<Components::BoxCollider>(player_idle_width * 2.0f, player_idle_height * 2.0f, true)
    .add_component<Components::Animation>(24, 0.1f)
    .add_component<Components::AnimationState>(player_anim_state)
    .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Health>(50, 100)
    .add_component<Components::HealthConfig>(Components::HealthConfig{ .drain_per_sec = 5.0f, .heal_while_moving_per_sec = 10.0f, .carry = 0.0f });

  auto floor = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(960.0f, 770.0f, 0.0f, 1920.0f, 100.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.3f, 0.8f, 0.3f, 1.0f)
    .add_component<Components::RigidBody>(true, false)
    .add_component<Components::BoxCollider>(1920.0f, 100.0f, false);

  // Create text entities
  title_text = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(50.0f, 50.0f, 0.0f, 1.0f, 1.0f)
    .add_component<Components::Font>(title_font)
    .add_component<Components::Text>("Engine Showcase")
    .add_component<Components::Color>(1.0f, 0.9f, 0.3f, 1.0f);

  info_text = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(50.0f, 140.0f, 0.0f, 0.6f, 0.6f)
    .add_component<Components::Font>(info_font)
    .add_component<Components::Text>("A/D: Move | Space: Jump | E: Attack | C: Collect")
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

  banana_count_text = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(50.0f, 200.0f, 0.0f, 0.7f, 0.7f)
    .add_component<Components::Font>(info_font)
    .add_component<Components::Text>("Bananas: 0")
    .add_component<Components::Color>(1.0f, 1.0f, 0.0f, 1.0f);

  enemy_health_text = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(50.0f, 240.0f, 0.0f, 0.7f, 0.7f)
    .add_component<Components::Font>(info_font)
    .add_component<Components::Text>("Enemy HP: 50/50")
    .add_component<Components::Color>(1.0f, 0.0f, 0.0f, 1.0f);

  player_health_text = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(50.0f, 280.0f, 0.0f, 0.7f, 0.7f)
    .add_component<Components::Font>(info_font)
    .add_component<Components::Text>("Player HP: 50/100")
    .add_component<Components::Color>(0.2f, 1.0f, 0.2f, 1.0f);

  player_hp_bg = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(
      hpbar_left + hpbar_w * 50.0f,
      hpbar_top + hpbar_h * 50.0f,
      0.0f, hpbar_w, hpbar_h)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.15f, 0.15f, 0.15f, 1.0f);

  player_hp_fill = Engine::ECS::create_entity()
    .add_component<Components::Tag<"ShowcaseState">>()
    .add_component<Components::Transform2D>(
      hpbar_left + (hpbar_w * 0.5f),
      hpbar_top + hpbar_h * 0.5f,
      0.0f, hpbar_w * 0.5f, hpbar_h)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.2f, 1.0f, 0.2f, 1.0f);

  player.add_component<Components::HealthBar>(Components::HealthBar{
    .bar_bg = player_hp_bg,
    .bar_fill = player_hp_fill,
    .text = player_health_text,
    .left = hpbar_left, .top = hpbar_top, .width = hpbar_w, .height = hpbar_h
    });

  // Subscribe to attack events
  Engine::EventManager::add_event_listener<AttackEvent>([this](const AttackEvent& event) {
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

      // Play punch sound
      Engine::Audio::play_sound(punch_sound);

      // Switch to punch animation state
      auto* anim_state = player.get_component<Components::AnimationState>();
      if (anim_state) {
        anim_state->set_entity_state(PUNCH_STATE);
        std::cout << "Playing punch animation\n";
      }
    }

    if (event.target == player.get_id()) {
      if (player.is_valid() && player.has_component<Components::Health>()) {
        auto* hp = player.get_component<Components::Health>();
        hp->heal(event.damage);
        if (player_health_text.is_valid() && player_health_text.has_component<Components::Text>()) {
          auto* text = player_health_text.get_component<Components::Text>();
          text->content = "Player HP: " + std::to_string(hp->value) + "/" + std::to_string(hp->max);
        }
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

        // Update enemy health text
        if (enemy_health_text.is_valid() && enemy_health_text.has_component<Components::Text>()) {
          auto* text = enemy_health_text.get_component<Components::Text>();
          text->content = "Enemy HP: " + std::to_string(health->value) + "/" + std::to_string(health->max);
        }

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
  Engine::EventManager::add_event_listener<CollectEvent>([this](const CollectEvent& event) {
    std::cout << "Collect event! Entity " << event.collector
      << " collected Entity " << event.collected
      << " for " << event.points << " points!\n";

    // Update score
    if (event.collector == player.get_id()) {
      banana_collected += event.points;

      // Update banana count text
      if (banana_count_text.is_valid() && banana_count_text.has_component<Components::Text>()) {
        auto* text = banana_count_text.get_component<Components::Text>();
        text->content = "Bananas: " + std::to_string(banana_collected);
      }
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

  // Start playing background music (looped)
  bgm_channel = Engine::Audio::play_sound(bgm_sound, 0.5f, true);
}

void ShowcaseState::update() {
  auto* player_transform = player.get_component<Components::Transform2D>();
  auto* rigid = player.get_component<Components::RigidBody>();
  auto* anim_state = player.get_component<Components::AnimationState>();

  if (!player_transform || !rigid || !anim_state) {
    if (walk_sound_channel != Engine::Audio::INVALID_CHANNEL) {
      Engine::Audio::stop_channel(walk_sound_channel);
      walk_sound_channel = Engine::Audio::INVALID_CHANNEL;
    }
    return;
  }

  // Only handle movement and animation changes if not punching
  if (anim_state->current_state != PUNCH_STATE) {
    // Reset forces before applying new ones
    rigid->force.zero();

    // Player movement input using physics forces
    bool moving_horizontally = false;

    if (Engine::Input::is_held(Engine::KeyCode::A)) {
      Engine::Systems::Physics2DSystem::walk_left(*rigid);
      facing_right = false;
      moving_horizontally = true;
    }
    if (Engine::Input::is_held(Engine::KeyCode::D)) {
      Engine::Systems::Physics2DSystem::walk_right(*rigid);
      facing_right = true;
      moving_horizontally = true;
    }

    // Jump with Space when on ground
    if (Engine::Input::is_held(Engine::KeyCode::Space) && rigid->on_ground && !jumping) {
      jumping = true;
      Engine::Systems::Physics2DSystem::jump(*rigid);
      Engine::Audio::play_sound(jump_sound);
    }

    // Reset jumping flag when space is released
    if (Engine::Input::is_released(Engine::KeyCode::Space) && jumping) {
      jumping = false;
    }

    // Animation change based on movement
    bool should_walk = moving_horizontally;

    if (should_walk && anim_state->current_state != RUN_STATE) {
      anim_state->set_entity_state(RUN_STATE);
      walk_sound_channel = Engine::Audio::play_sound(walk_sound, 1.0f, true);
    }
    else if (!should_walk && anim_state->current_state == RUN_STATE) {
      anim_state->set_entity_state(IDLE_STATE);

      if (walk_sound_channel != Engine::Audio::INVALID_CHANNEL) {
        Engine::Audio::stop_channel(walk_sound_channel);
        walk_sound_channel = Engine::Audio::INVALID_CHANNEL;
      }
    }
  }

  // Apply facing direction to scale (flip sprite horizontally)
  // The AnimationStateSystem handles base scale, we just apply the sign
  player_transform->scale.x = std::abs(player_transform->scale.x) * (facing_right ? 1.0f : -1.0f);

  // Attack enemy with E key (using event system)
  if (Engine::Input::is_pressed(Engine::KeyCode::E) && anim_state->current_state != PUNCH_STATE) {
    std::cout << "E key pressed! Player ID: " << player.get_id() << ", Enemy ID: " << enemy.get_id() << "\n";

    // Send attack event 
    Engine::EventManager::send_event<AttackEvent>(player.get_id(), enemy.get_id(), 10);

    if (auto* t = enemy.get_component<Components::Transform2D>()) {
      Engine::EventManager::send_event<Engine::Systems::SpawnParticlesEvent>(
        Math::Vec2D{ t->position.x, t->position.y }, 96);
    }
    // Punch animation will auto-transition to idle when done (handled by AnimationStateSystem)
  }
  // Collect banana with C key (using event system)
  if (Engine::Input::is_pressed(Engine::KeyCode::C)) {
    Engine::EventManager::send_event<CollectEvent>(player.get_id(), banana.get_id(), 1);
  }

  // Camera follow player
  auto* camera_comp = camera.get_component<Components::Camera>();
  if (camera_comp) {
    camera_comp->position.x = player_transform->position.x - Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().window_width / 2.0f;
    //camera_comp->position.y = player_transform->position.y - Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().window_height / 2.0f;
  }

}

void ShowcaseState::draw() {
  Engine::Graphics::clear_screen(background_texture);
}

void ShowcaseState::free() {
  // Stop any playing sounds
  if (walk_sound_channel != Engine::Audio::INVALID_CHANNEL) {
    Engine::Audio::stop_channel(walk_sound_channel);
    walk_sound_channel = Engine::Audio::INVALID_CHANNEL;
  }

  if (bgm_channel != Engine::Audio::INVALID_CHANNEL) {
    Engine::Audio::stop_channel(bgm_channel);
    bgm_channel = Engine::Audio::INVALID_CHANNEL;
  }

  // Cleanup entities
  Engine::ECS::destroy_entities_with<Components::Tag<"ShowcaseState">>();

  // Remove event listeners
  Engine::EventManager::remove_event_listener<AttackEvent>();
  Engine::EventManager::remove_event_listener<CollectEvent>();
}

void ShowcaseState::unload() {
  // Resources are automatically cleaned up when shared_ptrs go out of scope
  walk_sound.reset();
  jump_sound.reset();
  punch_sound.reset();
  bgm_sound.reset();

  // Clear unused resources from cache
  Engine::Resources::clear_unused_resources();
}