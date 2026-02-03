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
    // Register button callbacks
    Engine::FunctionRegistry<void()>::register_function("start_game", []() {
      Engine::SceneManager::load_scene("GameplayState");
      });
    Engine::FunctionRegistry<void()>::register_function("options_menu", []() {
      // Engine::SceneManager::push_scene("OptionsState");
      });
    Engine::FunctionRegistry<void()>::register_function("exit_game", []() {
      Engine::Core::stop();
      });

    // Create all main menu entities programmatically instead of loading from level

    // Background layers (back to front)
    // Entity 3: BackSky background
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png"));

    // Entity 4: MidBuildings
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/MidBuildings.png"));

    // Entity 5: FrontBuildings
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/FrontBuildings.png"));

    // Entity 6: Carriage
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/Carriage.png"));

    // Entity 7: MainMenuImpact (title/logo)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/MainMenuImpact.png"));

    // Entity 8: Play Button (with polygon collider)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/PlayButton.png"))
      .add_component<Components::BoxCollider>(std::vector<Math::Vec2D>{
      Math::Vec2D(-298.97f, -79.79f),
        Math::Vec2D(-275.49f, 68.05f),
        Math::Vec2D(173.73f, -178.52f),
        Math::Vec2D(165.90f, -447.70f)
    }, false)
      .add_component<Components::Clickable>("start_game", "", "");

    // Entity 9: Options Button (with polygon collider)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/OptionsButton.png"))
      .add_component<Components::BoxCollider>(std::vector<Math::Vec2D>{
      Math::Vec2D(551.86f, -379.65f),
        Math::Vec2D(524.46f, -54.79f),
        Math::Vec2D(3.91f, 90.02f),
        Math::Vec2D(-11.74f, -66.54f)
    }, false)
      .add_component<Components::Clickable>("options_menu", "", "");

    // Entity 10: Credits Button (with polygon collider)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/CreditsButton.png"))
      .add_component<Components::BoxCollider>(std::vector<Math::Vec2D>{
      Math::Vec2D(-272.52f, 7.33f),
        Math::Vec2D(-276.43f, 145.31f),
        Math::Vec2D(162.93f, 180.54f),
        Math::Vec2D(386.02f, -94.43f)
    }, false)
      .add_component<Components::Clickable>("", "", "");

    // Entity 11: Exit Button (with polygon collider)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/ExitButton.png"))
      .add_component<Components::BoxCollider>(std::vector<Math::Vec2D>{
      Math::Vec2D(-290.13f, 122.79f),
        Math::Vec2D(-325.35f, 213.81f),
        Math::Vec2D(-81.69f, 342.97f),
        Math::Vec2D(67.04f, 185.41f)
    }, false)
      .add_component<Components::Clickable>("exit_game", "", "");

    // Entity 12: Monkey character
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/Monkey.png"));

    // Entity 13: Bro character
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/Bro.png"));

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