#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/Systems/PhysicsSystem.h"
using namespace Engine;
using phy2D = Systems::Physics2DSystem;

namespace {
  bool jumping = false;
}
class PhysicsTestState : public Engine::GameState<PhysicsTestState> {
private:
  Engine::ECS::Entity prefab_instance;
  Engine::ECS::Entity player;

public:
  void load() {
    // Free falling box
    Vec2D player_pos(800.0f, 300.0f);
    player = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(player_pos.x, player_pos.y, 0.0f, 20.0f, 20.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(player_pos,true, true)
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
    Components::Transform2D*  player_xform  = player.get_component<Components::Transform2D>();
    Components::RigidBody*    player_rigid  = player.get_component<Components::RigidBody>();
    Components::BoxCollider*  player_box    = player.get_component<Components::BoxCollider>();

    if (!player_xform || !player_rigid || !player_box) return;

    // idt Input::is_pressed(KeyCode::A) || Input::is_held(KeyCode::A) and just is held only makes much of a difference

    player_rigid->force.zero();

    if (Input::is_pressed(KeyCode::A) || Input::is_held(KeyCode::A))
    {
      //walk left
      phy2D::walk_left(*player_rigid);
    }
    if (Input::is_held(KeyCode::D))
    {
      //walk right
      phy2D::walk_right(*player_rigid);
    }
    if (Input::is_held(KeyCode::Space) && player_rigid->on_ground && !jumping)
    {
      //jump
      std::cout << "JUMP\n";
      jumping = true;
      phy2D::jump(*player_rigid);
      std::cout << player_rigid->force << std::endl;
    }

    if (Input::is_released(KeyCode::Space) && jumping)
    {
      jumping = false;
    }
  }

  void draw() {
    Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.1f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"PhysicsTestState">>();
  };
};