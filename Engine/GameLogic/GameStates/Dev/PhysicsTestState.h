#pragma once

#include "../../Engine/MonkeyEngine.h"

class PhysicsTestState : public Engine::GameState<PhysicsTestState> {
private:
  Engine::ECS::Entity prefab_instance;

public:
  void load() {
    // Free falling box
    auto object1 = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(800.0f, 500.0f, 0.0f, 20.0f, 20.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(Vec2D(800.f, 500.f),true, true)
      .add_component<Components::BoxCollider>(20.0f, 20.0f, true); // width, height

    // Static ground box
    auto object2 = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(800.0f, 650.0f, 0.0f, 800.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(0.2f, 0.8f, 0.2f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(Vec2D(800.f, 650.f), false, false)
      .add_component<Components::BoxCollider>(800.0f, 50.0f, false); // width, height
  };

  void init() {};

  void update() 
  {
  }

  void draw() {
    Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.1f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"PhysicsTestState">>();
  };
};