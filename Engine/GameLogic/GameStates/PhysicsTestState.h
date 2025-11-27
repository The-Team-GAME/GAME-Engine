//-----------------------------------------------------------------------------------
// file:    PhysicsTestState.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a gamestate used to test out physics and collision logic.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Engine/MonkeyEngine.h"
#include "../Engine/Systems/PhysicsSystem.h"
#include "../Engine/Systems/LogicSystem.h"
using namespace Engine;
using phy2D = Systems::Physics2DSystem;
using logi = Systems::LogicSystem;

namespace {
  bool jumping = false;
  ECS::Entity player;
  Vec2D player_pos(800.0f, 100.0f);
  Vec2D npc_scale(20, 20);
  Vec4D white(1.0f, 1.0f, 1.0f, 1.0f);
  Vec4D grey(0.7f, 0.7f, 0.7f, 1.0f);
  Vec4D blue(0.2f, 0.2f, 1.0f, 1.0f);
  Vec4D red(1.0f, 0.2f, 0.2f, 1.0f);

  // No rotate parameter for my uses
  ECS::Entity make_player(Vec2D position, Vec2D scale)
  {
    ECS::Entity ret = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(position.x, position.y, 0.0f, scale.x, scale.y) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(blue)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(scale.x, scale.y, false) // width, height
      .add_component<Components::Logic>(IDLE)
      .add_component<Components::Player_>();
    return ret;
  }

  ECS::Entity make_npc(Vec2D position, Vec2D scale, f32 left_chase_bound, f32 left_patrol_bound, f32 right_patrol_bound, f32 right_chase_bound)
  {
    ECS::Entity ret = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(position.x, position.y, 0.0f, scale.x, scale.y) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(red)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(scale.x, scale.y, false) // width, height
      .add_component<Components::Logic>(PATROL, left_chase_bound, left_patrol_bound, right_patrol_bound, right_chase_bound);
    return ret;
  }

  void make_wall(Vec2D position, Vec2D scale)
  {
    f32 rotation = 0.f;
    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(position.x, position.y, rotation, scale.x, scale.y) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(white)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(scale.x, scale.y,  rotation, true); // width, height
  }
}
class PhysicsTestState : public Engine::Scene {
private:
  Engine::ECS::Entity prefab_instance;

public:
  void load() {
    // Free falling box
    player = make_player(player_pos, npc_scale);
    Components::BoxCollider* player_box = player.get_component< Components::BoxCollider>();
    player_box->kinetic_friction = 0.1f;
    make_npc(Vec2D{ 1000.f, 825.f }, npc_scale, 600.f, 750.f, 1050.f, 1200.f);

    // Walling the player in
    make_wall(Vec2D(800.0f, 875.0f), Vec2D(1600.0f, 50.0f));
    make_wall(Vec2D(800.0f, 25.0f) , Vec2D(1600.0f, 50.0f));
    make_wall(Vec2D(25.f, 450.0f)  , Vec2D(50.0f, 900.0f));
    make_wall(Vec2D(1575.f, 450.0f), Vec2D(50.0f, 900.0f));

    // Platforms to jump on

    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(100.0f, 725.0f, 0.0f, 100.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(100.0f, 50.0f, true); // width, height

    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(150.0f, 775.0f, 0.0f, 200.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(200.0f, 50.0f, true); // width, height

    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(200.0f, 825.0f, 0.0f, 300.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(300.0f, 50.0f, true); // width, height

    ECS::Entity slope = Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(500.0f, 675.0f, 30.0f, 500.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(500.0f, 50.0f, 30.f, true); // width, height
    Components::BoxCollider* box_slope = slope.get_component< Components::BoxCollider>();
    box_slope->kinetic_friction = 0.9f;
    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(1300.0f, 675.0f, 0.0f, 300.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(300.0f, 50.0f, true); // width, height

    // the jummping blocks
    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(300.0f, 375.0f, 0.0f, 50.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(50.0f, 50.0f, true); // width, height

    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(500.0f, 450.0f, 0.0f, 50.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(50.0f, 50.0f, true); // width, height

    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(700.0f, 525.0f, 0.0f, 50.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(50.0f, 50.0f, true); // width, height

    Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
      .add_component<Components::Transform2D>(900.0f, 600.0f, 0.0f, 50.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
      .add_component<Components::Color>(grey)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(50.0f, 50.0f, true); // width, height
  };

  void init() {};

  void update() 
  {
    Components::Transform2D*  player_xform  = player.get_component<Components::Transform2D>();
    Components::RigidBody*    player_rigid  = player.get_component<Components::RigidBody>();
    Components::BoxCollider*  player_box    = player.get_component<Components::BoxCollider>();

    if (!player_xform || !player_rigid || !player_box) return;

    logi::take_input(player);
    
    // idt Input::is_pressed(KeyCode::A) || Input::is_held(KeyCode::A) and just is held only makes much of a difference

    //player_rigid->force.zero();

    //if (Input::is_pressed(KeyCode::A) || Input::is_held(KeyCode::A))
    //{
    //  //walk left
    //  phy2D::walk_left(*player_rigid);
    //}
    //if (Input::is_held(KeyCode::D))
    //{
    //  //walk right
    //  phy2D::walk_right(*player_rigid);
    //}
    //if (Input::is_held(KeyCode::Space) && player_rigid->on_ground && !jumping)
    //{
    //  //jump
    //  // std::cout << "JUMP\n";
    //  jumping = true;
    //  phy2D::jump(*player_rigid);
    //  std::cout << player_rigid->force << std::endl;
    //}

    //if (Input::is_released(KeyCode::Space) && jumping)
    //{
    //  jumping = false;
    //}

  }

  void draw() {
    Engine::Graphics::clear_screen({ 0.1f, 0.1f, 0.1f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"PhysicsTestState">>();
  };
};