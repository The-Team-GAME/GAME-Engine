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
    camera.add_component<Components::Parent>(player_entity);

    player_entity.add_component<Components::HeartHUDSprite>();

  };

  void init() {};

  void update() {};

  void draw() {};

  void free() {};

  void unload() {
    Engine::Audio::stop_all_sounds();
  };
};