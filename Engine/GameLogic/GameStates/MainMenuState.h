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
#include "../../Engine/Easing.h"

void button_scale(Engine::ECS::Entity entity) {
  Vec2D& scale = entity.get_component<Components::Transform2D>()->scale;

  const Vec2D og_scale = Vec2D(192.0f * 1.5f, 64.0f * 1.5f);
  const Vec2D target_scale = og_scale * 1.2f;

  // Using BackOut easing for smooth scale-up with slight overshoot
  float easedX = Engine::Easing::EaseBackOut(0.10f, scale.x, target_scale.x - scale.x, 1.0f);
  float easedY = Engine::Easing::EaseBackOut(0.10f, scale.y, target_scale.y - scale.y, 1.0f);

  scale = Vec2D(easedX, easedY);
}

void button_exit_scale(Engine::ECS::Entity entity) {
  Vec2D& scale = entity.get_component<Components::Transform2D>()->scale;

  const Vec2D og_scale = Vec2D(192.0f * 1.5f, 64.0f * 1.5f);

  // Using CubicOut easing for smooth scale-down
  float t = 0.10f;
  float easedX = Engine::Easing::EaseCubicOut(t, scale.x, og_scale.x - scale.x, 1.0f);
  float easedY = Engine::Easing::EaseCubicOut(t, scale.y, og_scale.y - scale.y, 1.0f);

  scale = Vec2D(easedX, easedY);
}


class MainMenuState : public Engine::Scene
{
private:

public:
  void load() {
    Engine::ECS::create_entity()
      .add_component<Components::Tag<"MainMenuState">>()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/MAIN MENU.png"))
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_to_layer("background");

    // Start Button
    Engine::ECS::create_entity()
      .add_component<Components::Tag<"MainMenuState">>()
      .add_component<Components::Transform2D>(625.0f, 700.0f, 0.0f, 192.0f * 1.5f, 64.0f * 1.5f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Button_Start.png"))
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Clickable>([]()
        {
          Engine::SceneManager::load_scene("LevelTestState");
        },

        [](Engine::ECS::Entity entity) { //Scale/Descale
          button_scale(entity);
        },
        [](Engine::ECS::Entity entity) { //Unscale
          button_exit_scale(entity);
        })
      .add_to_layer("ui");

    // Exit Button
    Engine::ECS::create_entity()
      .add_component<Components::Tag<"MainMenuState">>()
      .add_component<Components::Transform2D>(975.0f, 700.0f, 0.0f, 192.0f * 1.5f, 64.0f * 1.5f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Button_Exit.png"))
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Clickable>([]()
        {
          Engine::Core::stop();
        },
        [](Engine::ECS::Entity entity) {
          button_scale(entity);
        },
        [](Engine::ECS::Entity entity) {
          button_exit_scale(entity);
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