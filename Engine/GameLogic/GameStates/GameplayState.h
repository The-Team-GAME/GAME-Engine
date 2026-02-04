//-----------------------------------------------------------------------------------
// file:    GameplayState.h
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

#include "../Components/GameplayComponent.h"
#include "../Components/InteractableComponents.h"


class GameplayState : public Engine::Scene
{
  Engine::Audio::Sound bgm;

public:
  void load() {
    Engine::Resources::open_level("levels/LevelOne.json");

    bgm = Engine::Resources::load_sound("sounds/BGM/Level 1 BGM.wav");
    Engine::Audio::play_sound(bgm, 0.3f, true); // TODO: get from settings

    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    // Camera entity
    Engine::ECS::Entity camera = Engine::ECS::create_entity();
    camera.add_component<Components::Transform2D>(0.0f, 200.0f, 0.0f, 1.0f, 1.0f);
    camera.add_component<Components::Camera>("player camera", true);
    camera.add_component<Components::Parent>(player_entity.get_id());

    player_entity.add_component<Components::HeartHUDSprite>();

    // Victory zone - triggers victory screen when player enters
    // TODO: Position this at the end of the level
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(14600.0f, 830.0f, 0.0f, 100.0f, 200.0f)
      .add_component<Components::Interactable>(50.0f, "show_victory", "", "")
      .add_to_layer("effect");

    // Old man NPC - press E to interact and show dialogue
    auto old_man = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(200.0f, 800.0f, 0.0f, 64.0f, 64.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/NPCOldman.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Interactable>(100.0f, "", "old_man_on_update", "old_man_on_exit")
      .add_to_layer("player");
    Log::info("Old man entity created with ID: {}", old_man.get_id());

  };

  void init() {};

  void update() {

    // Check if pause button is pressed
    if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
      Engine::SceneManager::push_scene("PauseState");
    }

  };

  void draw() {};

  void free() {};

  void unload() {
    Engine::Audio::stop_all_sounds();
  };
};