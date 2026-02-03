//-----------------------------------------------------------------------------------
// file:    GameplayCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Level callbacks for gameplay levels. Registers on_init, on_update, and
//          on_exit functions that replace the old GameplayState scene.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"

namespace LevelCallbacks {
  namespace Gameplay {

    namespace {
      Engine::Audio::Sound bgm;
    }

    inline void on_init() {
      bgm = Engine::Resources::load_sound("sounds/BGM/Level 1 BGM.wav");
      Engine::Audio::play_sound(bgm, 0.3f, true); // TODO: get from settings

      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

      if (player_entity.is_valid()) {
        // Camera entity
        Engine::ECS::Entity camera = Engine::ECS::create_entity();
        camera.add_component<Components::Transform2D>(0.0f, 200.0f, 0.0f, 1.0f, 1.0f);
        camera.add_component<Components::Camera>("player camera", true);
        camera.add_component<Components::Parent>(player_entity.get_id());

        player_entity.add_component<Components::HeartHUDSprite>();
      }
    }

    inline void on_update() {
      // Check if pause button is pressed
      if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
        // TODO: Implement pause as an overlay level or keep scene for UI overlays
        Engine::SceneManager::push_scene("PauseState");
      }
    }

    inline void on_exit() {
      Engine::Audio::stop_all_sounds();
    }

    // Register all gameplay level callbacks
    inline void register_callbacks() {
      Engine::FunctionRegistry<void()>::register_function("Gameplay_Init", on_init);
      Engine::FunctionRegistry<void()>::register_function("Gameplay_Update", on_update);
      Engine::FunctionRegistry<void()>::register_function("Gameplay_Exit", on_exit);
    }

  } // namespace Gameplay
} // namespace LevelCallbacks
