//-----------------------------------------------------------------------------------
// file:    StressTestState.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of StressTestState that spawns 1000 animated sprites
//          with ability to switch between lazy monkey and player sprites.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "StressTestState.h"

void StressTestState::load() {
  // Load sprite sheet textures
  lazy_monkey_texture = Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Walk_Spritesheet.png");
  player_run_texture = Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png");
}

void StressTestState::init() {
  // Spawn 1000 animated sprites
  for (s32 i = 0; i < 1000; ++i) {
    f32 x = static_cast<f32>(rand() % 1600); // Random x position within window width
    f32 y = static_cast<f32>(rand() % 900);  // Random y position within window height
    f32 size = static_cast<f32>((rand() % 40) + 80); // Random size between 80 and 120 (bigger)
    f32 rotation = static_cast<f32>(rand() % 360); // Random rotation angle

    // Random color variation
    Components::Color color = Components::Color(
      0.8f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX)) * 0.2f,
      0.8f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX)) * 0.2f,
      0.8f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX)) * 0.2f,
      1.0f
    );

    // Create AnimationState component with both states
    Components::AnimationState anim_state;

    // Add monkey state
    anim_state.add_state(
      MONKEY_STATE,
      Components::Texture(lazy_monkey_texture),
      Components::TextureAtlas(monkey_frame_width, monkey_frame_height, 1, monkey_frame_count, monkey_frame_count),
      Components::Animation(monkey_frame_count, monkey_anim_speed)
    );

    // Add player state
    anim_state.add_state(
      PLAYER_STATE,
      Components::Texture(player_run_texture),
      Components::TextureAtlas(player_frame_width, player_frame_height, 1, player_frame_count, player_frame_count),
      Components::Animation(player_frame_count, player_anim_speed)
    );

    // Set initial state to monkey
    anim_state.set_entity_state(MONKEY_STATE);

    // Create entity with AnimationState component
    auto entity = Engine::ECS::create_entity()
      .add_component<Components::Tag<"StressTestState">>()
      .add_component<Components::Transform2D>(x, y, rotation, size, size)
      .add_component<Components::Color>(color)
      .add_component<Components::Texture>(lazy_monkey_texture)
      .add_component<Components::TextureAtlas>(monkey_frame_width, monkey_frame_height, 1, monkey_frame_count, monkey_frame_count)
      .add_component<Components::Animation>(monkey_frame_count, monkey_anim_speed)
      .add_component<Components::AnimationState>(anim_state)
      .add_component<Components::RigidBody>(false, false);

    // Random rotation speed
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) {
      rigid->angular_velocity = 30.0f + static_cast<f32>(rand() % 90); // Random rotation speed between 30 and 120 degrees per second
    }
  }
}

void StressTestState::update() {
  // Press SPACE to toggle between monkey and player sprites
  if (Engine::Input::is_pressed(Engine::KeyCode::Space)) {
    // Update all entities with the new animation state
    Engine::ECS::for_each_entity_with<Components::Tag<"StressTestState">, Components::AnimationState>(
      [this](Engine::ECS::Entity entity) {
        auto* anim_state = entity.get_component<Components::AnimationState>();
        if (anim_state) {
          // Toggle between monkey and player states
          if (anim_state->current_state == MONKEY_STATE) {
            anim_state->set_entity_state(PLAYER_STATE);
          }
          else {
            anim_state->set_entity_state(MONKEY_STATE);
          }
        }
      });
  }
}

void StressTestState::draw() {
  Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.15f, 1.0f }); // Clear to dark blue-grey
}

void StressTestState::free() {
  // Cleanup entities
  Engine::ECS::destroy_entities_with<Components::Tag<"StressTestState">>();
}

void StressTestState::unload() {
  // Resources are automatically cleaned up when shared_ptrs go out of scope
  lazy_monkey_texture.reset();
  player_run_texture.reset();

  // Clear unused resources from cache
  Engine::Resources::clear_unused_resources();
}
