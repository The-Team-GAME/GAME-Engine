//-----------------------------------------------------------------------------------
// file:    BasicState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the BasicState which demonstrates basic rendering
//          capabilities including shapes, text, and audio playback. Serves as
//          a test state for fundamental engine features.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/* EVERY CODE HERE IS TEMPORARY */

#include "../../Engine/MonkeyEngine.h"

class MainMenuState : public Engine::Scene
{
private:

public:
  void load() {
    Engine::Resources::set_resource_path("Assets/");
    Engine::ECS::create_entity()
      .add_component<Components::Tag<"MainMenuState">>()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MAIN MENU.png"))
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_to_layer("background");


    Engine::ECS::create_entity()
      .add_component<Components::Tag<"MainMenuState">>()
      .add_component<Components::Transform2D>(800.0f, 700.0f, 0.0f, 192.0f * 1.5f, 64.0f * 1.5f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Button_Start.png"))
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Clickable>([]()
        {
          Engine::SceneManager::load_scene("ShowcaseState");
        })
      .add_to_layer("ui");
  };

  void init() {};

  void update() {};

  void draw() {
    Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });
  };

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"MainMenuState">>();
  };
};