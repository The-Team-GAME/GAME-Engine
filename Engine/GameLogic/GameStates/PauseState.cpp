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
  Engine::Graphics::Texture nothing_button_hovered;
  Engine::Graphics::Texture resume_button_hovered;
  Engine::Graphics::Texture restart_button_hovered;
  Engine::Graphics::Texture options_button_hovered;
  Engine::Graphics::Texture exit_button_hovered;

  Engine::ECS::Entity background_entity;
  Engine::ECS::Entity hovered_button_entity;

  Engine::ECS::Entity resume_collider_entity;
  Engine::ECS::Entity restart_collider_entity;
  Engine::ECS::Entity options_collider_entity;
  Engine::ECS::Entity exit_collider_entity;
}

void PauseState::load()
{
  Log::info("PauseState::load called");
  nothing_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/NothingSelectedPausedMenu.png");
  resume_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ResumeButton.png");
  restart_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/RestartButton.png");
  options_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/OptionsButtons.png");
  exit_button_hovered = Engine::Resources::load_texture("images/UI/In-Game Pause/States/ExitButton.png");

  // Get current camera position to center the UI
  auto& camera = Engine::Systems::CameraSystem::get_camera();
  float cam_x = camera.position.x + 800.0f;
  float cam_y = camera.position.y + 450.0f;

  background_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      cam_x, cam_y,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.5f)
    .add_to_layer("ui");

  // Only for displaying which button is hovered
  hovered_button_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      cam_x, cam_y,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::Texture>(nothing_button_hovered)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_to_layer("ui");


  // Invisible collider for detecting mouse position
  resume_collider_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      cam_x, cam_y - 400.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
    .add_component<Components::Clickable>("pause_menu_resume")
    .add_to_layer("ui");

  restart_collider_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      cam_x, cam_y - 200.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
    .add_component<Components::Clickable>("pause_menu_restart")
    .add_to_layer("ui");

  options_collider_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      cam_x, cam_y,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
    .add_component<Components::Clickable>("pause_menu_options")
    .add_to_layer("ui");

  exit_collider_entity = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(
      cam_x, cam_y + 200.0f,
      0.0f,
      1600.0f, 900.0f
    )
    .add_component<Components::BoxCollider>(600.0f, 100.0f, true)
    .add_component<Components::Clickable>("pause_menu_exit")
    .add_to_layer("ui");
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
  // Destroy entities created in load()
  Engine::ECS::destroy_entity(background_entity);
  Engine::ECS::destroy_entity(hovered_button_entity);
  Engine::ECS::destroy_entity(resume_collider_entity);
  Engine::ECS::destroy_entity(restart_collider_entity);
  Engine::ECS::destroy_entity(options_collider_entity);
  Engine::ECS::destroy_entity(exit_collider_entity);
}