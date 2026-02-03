//-----------------------------------------------------------------------------------
// file:    MainMenuState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/* EVERY CODE HERE IS TEMPORARY */

#include "../../Engine/MonkeyEngine.h"


class MainMenuState : public Engine::Scene
{
  void main_menu_quit() {
    Engine::Core::stop();
  }

  void main_menu_start() {
    Engine::SceneManager::load_scene("GameplayState");
  }

public:
  Engine::Audio::Sound bgm;

  void load() {

    // Background layers (back to front)
    // Entity 3: BackSky background
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png"))
      .add_to_layer("main_menu_backsky");

    // Entity 4: MidBuildings
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BuildingsCombined.png"))
      .add_to_layer("main_menu_midbuildings");

    // Entity 6: Carriage
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/Carriage.png"))
      .add_to_layer("main_menu_carriage");

    // Entity 7: MainMenuImpact (title/logo)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/MainMenuImpact.png"))
      .add_to_layer("ui_background");

    // Helper lambda to create a polygon collider with proper offset
    auto create_polygon_collider = [](const std::vector<Math::Vec2D>& vertices, Math::Vec2D offset = Math::Vec2D(0.0f, 0.0f)) {
      Components::BoxCollider collider(vertices, false);
      collider.offset = offset;
      return collider;
      };

    // Entity 8: Play Button (with polygon collider) - offset (0, 0)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/PlayButton.png"))
      .add_component<Components::BoxCollider>(create_polygon_collider(
        std::vector<Math::Vec2D>{
      Math::Vec2D(-298.97f, -79.79f),
        Math::Vec2D(-275.49f, 68.05f),
        Math::Vec2D(173.73f, -178.52f),
        Math::Vec2D(165.90f, -447.70f)
    }))
      .add_component<Components::Clickable>("start_game", "", "")
      .add_to_layer("ui");

    // Entity 9: Options Button (with polygon collider) - offset (0, 0)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/OptionsButton.png"))
      .add_component<Components::BoxCollider>(create_polygon_collider(
        std::vector<Math::Vec2D>{
      Math::Vec2D(551.86f, -379.65f),
        Math::Vec2D(524.46f, -54.79f),
        Math::Vec2D(3.91f, 90.02f),
        Math::Vec2D(-11.74f, -66.54f)
    }))
      .add_component<Components::Clickable>("options_menu", "", "")
      .add_to_layer("ui");

    // Entity 10: Credits Button (with polygon collider) - offset (315.07, 62.62)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/CreditsButton.png"))
      .add_component<Components::BoxCollider>(create_polygon_collider(
        std::vector<Math::Vec2D>{
      Math::Vec2D(-272.52f, 7.33f),
        Math::Vec2D(-276.43f, 145.31f),
        Math::Vec2D(162.93f, 180.54f),
        Math::Vec2D(386.02f, -94.43f)
    }, Math::Vec2D(315.07f, 62.62f)))
      .add_component<Components::Clickable>("", "", "")
      .add_to_layer("ui");

    // Entity 11: Exit Button (with polygon collider) - offset (293.54, 80.23)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/ExitButton.png"))
      .add_component<Components::BoxCollider>(create_polygon_collider(
        std::vector<Math::Vec2D>{
      Math::Vec2D(-290.13f, 122.79f),
        Math::Vec2D(-325.35f, 213.81f),
        Math::Vec2D(-81.69f, 342.97f),
        Math::Vec2D(67.04f, 185.41f)
    }, Math::Vec2D(293.54f, 80.23f)))
      .add_component<Components::Clickable>("exit_game", "", "")
      .add_to_layer("ui");

    // Entity 12: Monkey character
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/Monkey.png"))
      .add_to_layer("ui_foreground");

    // Entity 13: Bro character
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/Bro.png"))
      .add_to_layer("ui_foreground");

    // Load and play background music
    bgm = Engine::Resources::load_sound("sounds/BGM/Main Menu/Ricoche_LO-fi_Cut-Scene_Loop_120bpm.wav");
    Engine::Audio::play_sound(bgm, 0.3f, true); // TODO: get from settings
  };

  void init() {};

  void update() {};

  void draw() {
  };

  void free() {};

  void unload() {
    Engine::Audio::stop_all_sounds();
  };
};