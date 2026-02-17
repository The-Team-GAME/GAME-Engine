//-----------------------------------------------------------------------------------
// file:    VictoryState.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Victory screen state displayed when the player wins.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "pch.h"
#include "VictoryState.h"
#include "../../Engine/Systems/GraphicsSystem.h"

// Check if any key or mouse button was pressed this frame
static bool any_key_pressed() {
  // Check all keyboard keys
  for (int key = 32; key <= 348; ++key) { // GLFW_KEY_SPACE to GLFW_KEY_LAST
    if (Engine::Input::is_pressed(static_cast<Engine::KeyCode>(key))) {
      return true;
    }
  }
  return false;
}

void VictoryState::load()
{
  // Get current camera position to center the UI
  auto& camera = Engine::Systems::CameraSystem::get_camera();
  float cam_x = camera.position.x + 800.0f;
  float cam_y = camera.position.y + 450.0f;

  // Create black semi-transparent background covering whole screen
  auto background = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.5f)
    .add_to_layer("main_menu_4");
  background_id = background.get_id();

  // Create victory screen sprite
  auto victory_screen = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(cam_x, cam_y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/In-Game Pause/PrimitiveWin/VictoryScreen.png"))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_to_layer("main_menu_5");
  victory_screen_id = victory_screen.get_id();

  Engine::Time::set_time_scale(0.0001f); // can't set to 0 or input will not work
}

void VictoryState::init()
{
}

void VictoryState::update()
{
  // Press any key to return to main menu
  if (any_key_pressed()) {
    Engine::SceneManager::load_scene("MainMenuState");
  }
}

void VictoryState::draw()
{
}

void VictoryState::free()
{
}

void VictoryState::unload()
{
  // Destroy victory screen entities
  if (background_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(background_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(background_id));
  }
  if (victory_screen_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(victory_screen_id))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(victory_screen_id));
  }

  background_id = 0;
  victory_screen_id = 0;

  Engine::Time::set_time_scale(1.0f);
}
