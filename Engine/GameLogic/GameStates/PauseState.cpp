//-----------------------------------------------------------------------------------
// file:    PauseState.cpp
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
#include "PauseState.h"
#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/Engine/Systems/GraphicsSystem.h"

namespace {
  // Engine::Graphics::Texture nothing_button_hovered;
  // Engine::Graphics::Texture resume_button_hovered;
  // Engine::Graphics::Texture restart_button_hovered;
  // Engine::Graphics::Texture options_button_hovered;
  // Engine::Graphics::Texture exit_button_hovered;

  // Engine::ECS::Entity background_entity;

  // Engine::ECS::Entity nothing_button_entity;
  // Engine::ECS::Entity resume_collider_entity;
  // Engine::ECS::Entity restart_collider_entity;
  // Engine::ECS::Entity options_collider_entity;
  // Engine::ECS::Entity exit_collider_entity;
}

void PauseState::load()
{
  Engine::Resources::open_level("levels/Pause.json", true, true);


  // Get current camera position to center the UI
  auto& camera = Engine::Systems::CameraSystem::get_camera();
  float cam_x = camera.position.x + 800.0f;
  float cam_y = camera.position.y + 450.0f;

  // Find ALL Level entities and locate the Pause level specifically
  auto level_entities = Engine::ECS::get_entities_with<Components::Level>();

  for (auto& level_entity : level_entities) {
    // Check if this is the Pause level
    auto* level_comp = level_entity.get_component<Components::Level>();
    if (level_comp && level_comp->level_path == "levels/Pause.json") {
      // Move this entity to the camera position
      if (level_entity.has_component<Components::Transform2D>()) {
        auto* transform = level_entity.get_component<Components::Transform2D>();
        if (transform) {
          transform->position.x = cam_x;
          transform->position.y = cam_y;
        }
      }
    }
  }

  // nothing_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/NothingSelectedPausedMenu.png");
  // resume_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ResumeButton.png");
  // restart_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/RestartButton.png");
  // options_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/OptionsButtons.png");
  // exit_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ExitButton.png");

  // background_entity = Engine::ECS::create_entity()
  //   .add_component<Components::Transform2D>(
  //     cam_x, cam_y,
  //     0.0f,
  //     1600.0f, 900.0f
  //   )
  //   .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
  //   .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.5f)
  //   .add_to_layer("ui_background"); 

  // // Only for displaying which button is hovered
  // nothing_button_entity = Engine::ECS::create_entity()
  //   .add_component<Components::Transform2D>(
  //     cam_x, cam_y,
  //     0.0f,
  //     1600.0f, 900.0f
  //   )
  //   .add_component<Components::Texture>(nothing_button_hovered)
  //   .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
  //   .add_to_layer("ui");  

  // // Invisible collider for detecting mouse position
  // resume_collider_entity = Engine::ECS::create_entity()
  //   .add_component<Components::Transform2D>(
  //     cam_x, cam_y,
  //     0.0f,
  //     1600.0f, 900.0f
  //   )
  //   .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
  //   .add_component<Components::Clickable>("pause_menu_resume", "pause_menu_button_hover", "pause_menu_button_exit_hover")
  //   .add_component<Components::Texture>(resume_button_hovered)
  //   .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
  //   .add_to_layer("ui_foreground");

  // restart_collider_entity = Engine::ECS::create_entity()
  //   .add_component<Components::Transform2D>(
  //     cam_x, cam_y,
  //     0.0f,
  //     1600.0f, 900.0f
  //   )
  //   .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
  //   .add_component<Components::Clickable>("pause_menu_restart", "pause_menu_button_hover", "pause_menu_button_exit_hover")
  //   .add_component<Components::Texture>(restart_button_hovered)
  //   .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
  //   .add_to_layer("ui_foreground");

  // options_collider_entity = Engine::ECS::create_entity()
  //   .add_component<Components::Transform2D>(
  //     cam_x, cam_y,
  //     0.0f,
  //     1600.0f, 900.0f
  //   )
  //   .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
  //   .add_component<Components::Clickable>("pause_menu_options", "pause_menu_button_hover", "pause_menu_button_exit_hover")
  //   .add_component<Components::Texture>(options_button_hovered)
  //   .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
  //   .add_to_layer("ui_foreground");

  // exit_collider_entity = Engine::ECS::create_entity()
  //   .add_component<Components::Transform2D>(
  //     cam_x, cam_y,
  //     0.0f,
  //     1600.0f, 900.0f
  //   )
  //   .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
  //   .add_component<Components::Clickable>("pause_menu_exit", "pause_menu_button_hover", "pause_menu_button_exit_hover")
  //   .add_component<Components::Texture>(exit_button_hovered)
  //   .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
  //   .add_to_layer("ui_foreground");

  Engine::Time::set_time_scale(0.0001f); // cant set to 0 or it input will not work
}

void PauseState::init()
{
}

void PauseState::update()
{
}

void PauseState::draw()
{
}

void PauseState::free()
{
}

void PauseState::unload()
{
  // Destroy all entities belonging to the Pause level
  Engine::LevelManager::clear_level_entities("levels/Pause.json");

  Engine::Time::set_time_scale(1.0f);
}