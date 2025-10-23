#pragma once

#include "../../Engine/MonkeyEngine.h"

class PhysicsTestState : public Engine::GameState<PhysicsTestState> {
private:
  Engine::ECS::Entity prefab_instance;

public:
  void load() {
    // Free falling box
    auto object1 = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(800.0f, 300.0f, 0.0f, 20.0f, 20.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(
        false,                    // is_kinematic (affected by physics)
        true,                     // detect_collisions (react with other bodies with collisions)
        true,                     // use_gravity (affected by gravity)
        1.0f,                     // mass
        0.0f,                     // rotation
        Math::Vec2D(0.0f, 0.0f),  // linear_velocity
        0.0f                      // angular_velocity
      )
      .add_component<Components::BoxCollider>(20.0f, 20.0f); // width, height

    // Static ground box
    auto object2 = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(800.0f, 650.0f, 0.0f, 800.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(0.2f, 0.8f, 0.2f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(
        true,                     // is_kinematic (not affected by physics)
        true,                     // detect_collisions (makes other bodies react with this body)
        false,                    // use_gravity (not affected by gravity)
        0.0f,                     // mass
        0.0f,                     // rotation
        Math::Vec2D(0.0f, 0.0f),  // linear_velocity
        0.0f                      // angular_velocity
      )
      .add_component<Components::BoxCollider>(800.0f, 50.0f); // width, height


  };

  void init() {};

  void update() {};

  void draw() {
    Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.1f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"PhysicsTestState">>();
  };
};