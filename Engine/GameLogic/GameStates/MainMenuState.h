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

  // Track whether we're showing the start screen or the actual menu
  bool showing_start_screen = true;

  // Store entity IDs for cleanup when transitioning
  std::vector<Engine::ECS::EntityID> start_screen_entities;

  // For manual animations
  Engine::ECS::EntityID carriage_parent_id = 0;
  Engine::ECS::EntityID text_any_key_id = 0;
  Engine::ECS::EntityID backsky_id = 0;
  Engine::ECS::EntityID main_menu_backsky_id = 0;
  float bounce_timer = 0.0f;
  float bounce_duration = 3.0f;
  float blink_timer = 0.0f;
  float blink_duration = 1.4f;
  float sky_timer = 0.0f;
  float sky_duration = 7.0f;

  // Check if any key or mouse button was pressed this frame
  bool any_key_pressed() {
    // Check all keyboard keys
    for (int key = 32; key <= 348; ++key) { // GLFW_KEY_SPACE to GLFW_KEY_LAST
      if (Engine::Input::is_pressed(static_cast<Engine::KeyCode>(key))) {
        return true;
      }
    }
    // // Check mouse buttons
    // if (Engine::Input::is_pressed(Engine::KeyCode::MouseLeft) ||
    //   Engine::Input::is_pressed(Engine::KeyCode::MouseRight) ||
    //   Engine::Input::is_pressed(Engine::KeyCode::MouseMiddle)) {
    //   return true;
    // }
    return false;
  }

  void create_start_screen() {
    // Entity 106: BackSky background (scrolling) - layer 0
    auto backsky = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(2400.0f, 450.0f), 0.0f, Math::Vec2D(4800.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png"))
      .add_to_layer("main_menu_0");
    start_screen_entities.push_back(backsky.get_id());
    backsky_id = backsky.get_id();

    // Entity 103: Parent entity for bounce animation (moves children up/down)
    auto carriage_parent = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(0.0f, 0.0f), 0.0f, Math::Vec2D(1.0f, 1.0f));
    start_screen_entities.push_back(carriage_parent.get_id());
    carriage_parent_id = carriage_parent.get_id();

    // Entity 100: BackCarriage - layer 2 (child of carriage_parent)
    auto back_carriage = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/BackCarriage.png"))
      .add_component<Components::Parent>(carriage_parent_id, Math::Vec2D(800.0f, 450.0f))
      .add_to_layer("main_menu_2");
    start_screen_entities.push_back(back_carriage.get_id());

    // Entity 99: Bro sleeping animation - layer 3 (child of carriage_parent)
    auto bro_sleeping = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(1360.0f, 785.0f), 0.0f, Math::Vec2D(433.33f, 433.33f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/Brosleepy3-Sheet.png"))
      .add_component<Components::TextureAtlas>(520, 520, 1, 8, 8) // 8 frames in a row
      .add_component<Components::Animation>(0, 7, 0.15f, true) // 8 frames, 0.15s each, looping
      .add_component<Components::Parent>(carriage_parent_id, Math::Vec2D(1360.0f, 785.0f))
      .add_to_layer("main_menu_3");
    start_screen_entities.push_back(bro_sleeping.get_id());

    // Entity 101: Front - layer 4 (child of carriage_parent)
    auto front = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/Front.png"))
      .add_component<Components::Parent>(carriage_parent_id, Math::Vec2D(800.0f, 450.0f))
      .add_to_layer("main_menu_4");
    start_screen_entities.push_back(front.get_id());

    // Entity 104: AnyKeyToStartBackground - layer 5
    auto any_key_bg = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/AnyKeyToStartBackground.png"))
      .add_to_layer("main_menu_5");
    start_screen_entities.push_back(any_key_bg.get_id());

    // Entity 105: TextAnyKey with blinking animation - layer 6
    auto text_any_key = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/TextAnyKey.png"))
      .add_to_layer("main_menu_6");
    start_screen_entities.push_back(text_any_key.get_id());
    text_any_key_id = text_any_key.get_id();
  }

  void destroy_start_screen() {
    for (auto id : start_screen_entities) {
      if (Engine::ECS::is_entity_active(Engine::ECS::Entity(id))) {
        Engine::ECS::destroy_entity(Engine::ECS::Entity(id));
      }
    }
    start_screen_entities.clear();
    carriage_parent_id = 0;
    text_any_key_id = 0;
    backsky_id = 0;
  }

  void create_main_menu() {
    // // Register button callbacks
    // Engine::FunctionRegistry<void()>::register_function("start_game", []() {
    //   Engine::SceneManager::load_scene("GameplayState");
    //   });
    // Engine::FunctionRegistry<void()>::register_function("options_menu", []() {
    //   // Engine::SceneManager::push_scene("OptionsState");
    //   });
    // Engine::FunctionRegistry<void()>::register_function("exit_game", []() {
    //   Engine::Core::stop();
    //   });

    // Background layers (back to front)
    // Entity 3: BackSky background (scrolling) - layer 0
    auto menu_backsky = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(2400.0f, 450.0f), 0.0f, Math::Vec2D(4800.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png"))
      .add_to_layer("main_menu_0");
    main_menu_backsky_id = menu_backsky.get_id();


    //TODO: ask darren to make moving buildings
    // // Entity 4: Buildings - layer 1
    // Engine::ECS::create_entity()
    //   .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
    //   .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    //   .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BuildingsCombined.png"))
    //   .add_to_layer("main_menu_1");

    // Entity 6: BackCarriage - layer 2
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/BackCarriage.png"))
      .add_to_layer("main_menu_2");

    // Entity 7: MainMenuImpact (title/logo) - layer 2
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/MainMenuImpact.png"))
      .add_to_layer("main_menu_2");

    // Helper lambda to create a polygon collider with proper offset
    auto create_polygon_collider = [](const std::vector<Math::Vec2D>& vertices, Math::Vec2D offset = Math::Vec2D(0.0f, 0.0f)) {
      Components::BoxCollider collider(vertices, false);
      collider.offset = offset;
      return collider;
      };

    // Entity 8: Play Button (with polygon collider) - layer 5
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
      .add_to_layer("main_menu_5");

    // Entity 9: Options Button (with polygon collider) - layer 5
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
      .add_to_layer("main_menu_5");

    // Entity 10: Credits Button (with polygon collider) - layer 5
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
      .add_to_layer("main_menu_5");

    // Entity 11: Exit Button (with polygon collider) - layer 5
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
      .add_to_layer("main_menu_5");

    // Entity 12: Monkey character - layer 3 (behind Front)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/Monkey.png"))
      .add_to_layer("main_menu_3");

    // Entity 13: Bro character - layer 3 (behind Front)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/UI/Bro.png"))
      .add_to_layer("main_menu_3");

    // Entity 14: Front overlay - layer 4 (in front of Bro and Monkey, behind buttons)
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/Front - no graffiti.png"))
      .add_to_layer("main_menu_4");
  }

public:
  Engine::Audio::Sound bgm;

  void load() {
    // Start with the start screen
    showing_start_screen = true;
    create_start_screen();

    // Load and play background music
    bgm = Engine::Resources::load_sound("sounds/BGM/Main Menu/Ricoche_LO-fi_Cut-Scene_Loop_120bpm.wav");
    Engine::Audio::play_sound(bgm, 0.3f, true); // TODO: get from settings
  };

  void init() {
    bounce_timer = 0.0f;
    blink_timer = 0.0f;
    sky_timer = 0.0f;
  };

  void update() {
    if (showing_start_screen) {
      float dt = Engine::Time::get_fixed_dt();

      // Sky scrolling animation (moves from right to left)
      sky_timer += dt;
      if (sky_timer >= sky_duration) {
        sky_timer -= sky_duration;
      }
      float sky_t = sky_timer / sky_duration;
      // Keyframes: 0->2400, 1->-800
      float sky_x = 2400.0f + ((-800.0f) - 2400.0f) * sky_t;

      if (backsky_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(backsky_id))) {
        auto* transform = Engine::ECS::Entity(backsky_id).get_component<Components::Transform2D>();
        if (transform) {
          transform->position.x = sky_x;
        }
      }

      // Bounce animation for carriage parent
      bounce_timer += dt;
      if (bounce_timer >= bounce_duration) {
        bounce_timer -= bounce_duration;
      }
      float bounce_t = bounce_timer / bounce_duration;
      float bounce_y = 0.0f;
      // Keyframes: 0->0, 0.368->0, 0.391->8.33, 0.416->8.33, 0.442->0, 0.5->0, 0.522->8.33, 0.546->8.33, 0.569->0, 1->0
      if (bounce_t < 0.368f) {
        bounce_y = 0.0f;
      }
      else if (bounce_t < 0.391f) {
        bounce_y = 8.33f * (bounce_t - 0.368f) / (0.391f - 0.368f);
      }
      else if (bounce_t < 0.416f) {
        bounce_y = 8.33f;
      }
      else if (bounce_t < 0.442f) {
        bounce_y = 8.33f * (1.0f - (bounce_t - 0.416f) / (0.442f - 0.416f));
      }
      else if (bounce_t < 0.5f) {
        bounce_y = 0.0f;
      }
      else if (bounce_t < 0.522f) {
        bounce_y = 8.33f * (bounce_t - 0.5f) / (0.522f - 0.5f);
      }
      else if (bounce_t < 0.546f) {
        bounce_y = 8.33f;
      }
      else if (bounce_t < 0.569f) {
        bounce_y = 8.33f * (1.0f - (bounce_t - 0.546f) / (0.569f - 0.546f));
      }
      else {
        bounce_y = 0.0f;
      }

      if (carriage_parent_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(carriage_parent_id))) {
        auto* transform = Engine::ECS::Entity(carriage_parent_id).get_component<Components::Transform2D>();
        if (transform) {
          transform->position.y = bounce_y;
        }
      }

      // Blink animation for "Press Any Key" text
      blink_timer += dt;
      if (blink_timer >= blink_duration) {
        blink_timer -= blink_duration;
      }
      float blink_t = blink_timer / blink_duration;
      float alpha = 1.0f;
      // Keyframes: 0->1, 0.306->1, 0.322->0, 0.671->0, 0.688->1, 1->1
      if (blink_t < 0.306f) {
        alpha = 1.0f;
      }
      else if (blink_t < 0.322f) {
        alpha = 1.0f - (blink_t - 0.306f) / (0.322f - 0.306f);
      }
      else if (blink_t < 0.671f) {
        alpha = 0.0f;
      }
      else if (blink_t < 0.688f) {
        alpha = (blink_t - 0.671f) / (0.688f - 0.671f);
      }
      else {
        alpha = 1.0f;
      }

      if (text_any_key_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(text_any_key_id))) {
        auto* color = Engine::ECS::Entity(text_any_key_id).get_component<Components::Color>();
        if (color) {
          color->a = alpha;
        }
      }

      // Check for any key press to transition to main menu
      if (any_key_pressed()) {
        destroy_start_screen();
        create_main_menu();
        showing_start_screen = false;
      }
    }
    else {
      // Main menu sky scrolling animation
      float dt = Engine::Time::get_fixed_dt();
      sky_timer += dt;
      if (sky_timer >= sky_duration) {
        sky_timer -= sky_duration;
      }
      float sky_t = sky_timer / sky_duration;
      float sky_x = 2400.0f + ((-800.0f) - 2400.0f) * sky_t;

      if (main_menu_backsky_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(main_menu_backsky_id))) {
        auto* transform = Engine::ECS::Entity(main_menu_backsky_id).get_component<Components::Transform2D>();
        if (transform) {
          transform->position.x = sky_x;
        }
      }
    }
  };

  void draw() {
  };

  void free() {};

  void unload() {
    Engine::Audio::stop_all_sounds();
    start_screen_entities.clear();
  };
};