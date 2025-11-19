//-----------------------------------------------------------------------------------
// file:    LevelTestState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a an empty state, useful for testing
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

class LevelTestState : public Engine::Scene
{
private:
public:
  void load() {
    auto level = Engine::Resources::open_level("levels/Example.json");
  };

  void init() {
    // now deserialized from level
    // Engine::ECS::create_entity().add_component<Components::Tag<"PhysicsTestState">>()
    //   .add_component<Components::Transform2D>(0.0f, 0.0f, 0.0f, 50.0f, 50.0f) // pos.x, pos.y, rotation, scale.x, scale.y
    //   .add_component<Components::Color>(blue)
    //   .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    //   .add_component<Components::RigidBody>(Components::RigidBody(false, true))
    //   .add_component<Components::BoxCollider>(50.0f, 50.0f, false) // width, height
    //   .add_component<Components::Logic>(IDLE)
    //   .add_component<Components::Player>()
    //   .remove_from_layer("default")
    //   .add_to_layer("player");
  };

  void update() {};

  void draw() {
    Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });
  };

  void free() {};

  void unload() {
    // destroy all entities
    Engine::ECS::destroy_all_entities();

  };
};