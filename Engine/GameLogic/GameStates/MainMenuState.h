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
    // Entity 106: BackSky background (scrolling) - ui_background layer
    auto backsky = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(4800.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png"))
      .add_to_layer("ui_background");
    start_screen_entities.push_back(backsky.get_id());

    // Entity 103: Parent entity with bounce animator (moves children up/down)
    Components::Animator bounce_animator;
    bounce_animator.duration = 3.0f;
    bounce_animator.is_looping = true;
    bounce_animator.is_playing = true;

    Components::AnimationTrack bounce_track;
    bounce_track.component_type = "Components::Transform2D";
    bounce_track.field_path = "position.y";
    bounce_track.keyframes = {
      Components::FloatKeyframe(0.0f, 0.0f, "EaseLinearNone"),
      Components::FloatKeyframe(0.0f, 0.368f, "EaseLinearNone"),
      Components::FloatKeyframe(8.33f, 0.391f, "EaseLinearNone"),
      Components::FloatKeyframe(8.33f, 0.416f, "EaseLinearNone"),
      Components::FloatKeyframe(0.0f, 0.442f, "EaseLinearNone"),
      Components::FloatKeyframe(0.0f, 0.5f, "EaseLinearNone"),
      Components::FloatKeyframe(8.33f, 0.522f, "EaseLinearNone"),
      Components::FloatKeyframe(8.33f, 0.546f, "EaseLinearNone"),
      Components::FloatKeyframe(0.0f, 0.569f, "EaseLinearNone"),
      Components::FloatKeyframe(0.0f, 1.0f, "EaseLinearNone")
    };
    bounce_animator.tracks.push_back(bounce_track);

    auto carriage_parent = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(0.0f, 0.0f), 0.0f, Math::Vec2D(1.0f, 1.0f))
      .add_component<Components::Animator>(bounce_animator);
    start_screen_entities.push_back(carriage_parent.get_id());
    auto carriage_parent_id = carriage_parent.get_id();

    // Entity 100: BackCarriage - ui layer (child of carriage_parent)
    auto back_carriage = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 369.62f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/BackCarriage.png"))
      .add_component<Components::Parent>(carriage_parent_id, Math::Vec2D(800.0f, 369.62f))
      .add_to_layer("ui");
    start_screen_entities.push_back(back_carriage.get_id());

    // Entity 99: Bro sleeping animation - ui layer (child of carriage_parent)
    auto bro_sleeping = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(1211.25f, 693.13f), 0.0f, Math::Vec2D(433.33f, 433.33f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/Brosleepy3-Sheet.png"))
      .add_component<Components::TextureAtlas>(520, 520, 1, 8, 8) // 8 frames in a row
      .add_component<Components::Animation>(0, 7, 0.15f, true) // 8 frames, 0.15s each, looping
      .add_component<Components::Parent>(carriage_parent_id, Math::Vec2D(1211.25f, 693.13f))
      .add_to_layer("ui");
    start_screen_entities.push_back(bro_sleeping.get_id());

    // Entity 101: Front - ui_foreground layer (child of carriage_parent)
    auto front = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/Front.png"))
      .add_component<Components::Parent>(carriage_parent_id, Math::Vec2D(800.0f, 450.0f))
      .add_to_layer("ui_foreground");
    start_screen_entities.push_back(front.get_id());

    // Entity 104: AnyKeyToStartBackground - ui_foreground layer
    auto any_key_bg = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/AnyKeyToStartBackground.png"))
      .add_to_layer("ui_foreground");
    start_screen_entities.push_back(any_key_bg.get_id());

    // Entity 105: TextAnyKey with blinking animation - ui_foreground layer
    Components::Animator blink_animator;
    blink_animator.duration = 1.4f;
    blink_animator.is_looping = true;
    blink_animator.is_playing = true;

    Components::AnimationTrack alpha_track;
    alpha_track.component_type = "Components::Color";
    alpha_track.field_path = "a";
    alpha_track.keyframes = {
      Components::FloatKeyframe(1.0f, 0.0f, "EaseLinearNone"),
      Components::FloatKeyframe(1.0f, 0.306f, "EaseLinearNone"),
      Components::FloatKeyframe(0.0f, 0.322f, "EaseLinearNone"),
      Components::FloatKeyframe(0.0f, 0.671f, "EaseLinearNone"),
      Components::FloatKeyframe(1.0f, 0.688f, "EaseLinearNone"),
      Components::FloatKeyframe(1.0f, 1.0f, "EaseLinearNone")
    };
    blink_animator.tracks.push_back(alpha_track);

    auto text_any_key = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu (Start)/TextAnyKey.png"))
      .add_component<Components::Animator>(blink_animator)
      .add_to_layer("ui_foreground");
    start_screen_entities.push_back(text_any_key.get_id());
  }

  void destroy_start_screen() {
    for (auto id : start_screen_entities) {
      if (Engine::ECS::is_entity_active(Engine::ECS::Entity(id))) {
        Engine::ECS::destroy_entity(Engine::ECS::Entity(id));
      }
    }
    start_screen_entities.clear();
  }

  void create_main_menu() {
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

    // Background layers (back to front)
    // Entity 3: BackSky background
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BackSky.png"))
      .add_to_layer("main_menu_backsky");

    // Entity 4: Buildings
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(Math::Vec2D(800.0f, 450.0f), 0.0f, Math::Vec2D(1600.0f, 900.0f))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MainMenu/BackGround/BuildingsCombined.png"))
      .add_to_layer("main_menu_buildings");

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
  };

  void update() {
    if (showing_start_screen) {
      // Animator system handles the blinking animation
      // Check for any key press to transition to main menu
      if (any_key_pressed()) {
        destroy_start_screen();
        create_main_menu();
        showing_start_screen = false;
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