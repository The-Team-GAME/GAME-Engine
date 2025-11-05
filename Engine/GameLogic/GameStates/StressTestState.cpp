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
  Engine::Resources::set_resource_path("Assets/");

  // Load sprite sheet textures
  lazy_monkey_texture = Engine::Resources::load_texture("images/LazyMonkey_Walk_Spritesheet.png");
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

    // Create entity with lazy monkey texture initially
    auto entity = Engine::ECS::create_entity()
      .add_component<Components::Tag<"StressTestState">>()
      .add_component<Components::Transform2D>(x, y, rotation, size, size)
      .add_component<Components::Color>(color)
      .add_component<Components::Texture>(lazy_monkey_texture)
      .add_component<Components::TextureAtlas>(monkey_frame_width, monkey_frame_height, 1, monkey_frame_count, monkey_frame_count)
      .add_component<Components::Animation>(monkey_frame_count, monkey_anim_speed)
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
    using_monkey_sprite = !using_monkey_sprite;

    // Update all entities with the new sprite
    Engine::ECS::for_each_entity_with<Components::Tag<"StressTestState">, Components::Texture, Components::TextureAtlas, Components::Animation>(
      [this](Engine::ECS::Entity entity) {
        auto* texture = entity.get_component<Components::Texture>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();
        auto* animation = entity.get_component<Components::Animation>();

        if (texture && atlas && animation) {
          if (using_monkey_sprite) {
            // Switch to lazy monkey sprite
            texture->texture_handle = lazy_monkey_texture;
            *atlas = Components::TextureAtlas(monkey_frame_width, monkey_frame_height, 1, monkey_frame_count, monkey_frame_count);
            *animation = Components::Animation(monkey_frame_count, monkey_anim_speed);
          }
          else {
            // Switch to player running sprite
            texture->texture_handle = player_run_texture;
            *atlas = Components::TextureAtlas(player_frame_width, player_frame_height, 1, player_frame_count, player_frame_count);
            *animation = Components::Animation(player_frame_count, player_anim_speed);
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
