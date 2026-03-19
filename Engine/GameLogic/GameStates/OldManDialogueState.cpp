//-----------------------------------------------------------------------------------
// file:    OldManDialogueState.cpp
// project: Monkey Stole My GPA
//
// brief:   See OldManDialogueState.h
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "OldManDialogueState.h"

namespace {
  constexpr const char* OLD_MAN_DIALOGUE_TEXT =
    "Its time to retrieve your homework boy! Reach the end of the station and take down the big monkey FOR YOUR GPA!";
}

bool OldManDialogueState::any_input_pressed() const
{
  for (int key = 32; key <= 348; ++key) {
    if (Engine::Input::is_pressed(static_cast<Engine::KeyCode>(key))) {
      return true;
    }
  }

  return Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)
    || Engine::Input::is_pressed(Engine::KeyCode::MouseRight)
    || Engine::Input::is_pressed(Engine::KeyCode::MouseMiddle);
}

void OldManDialogueState::load()
{
  is_open = true;

  dialogue_font_ = Engine::Resources::load_font("fonts/FiveFontsatFreddy's-Regular.ttf", 48);
  continue_font_ = Engine::Resources::load_font("fonts/FiveFontsatFreddy's-Regular.ttf", 32);

  dimmer_id_ = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(SCREEN_CENTER_X, SCREEN_CENTER_Y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.55f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_4")
    .get_id();

  dialogue_bg_id_ = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(SCREEN_CENTER_X, SCREEN_CENTER_Y, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/NotBroDialogue.png"))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_5")
    .get_id();

  npc_portrait_id_ = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(SCREEN_CENTER_X + 509.0f, SCREEN_CENTER_Y + 173.0f, 0.0f, 320.0f, 320.0f)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/NPCOldman.png"))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_6")
    .get_id();

  Engine::Time::set_time_scale(0.0001f);
}

void OldManDialogueState::init() {}

void OldManDialogueState::update()
{
  if (any_input_pressed()) {
    Engine::SceneManager::pop_scene();
  }
}

void OldManDialogueState::draw()
{
  // Render dialogue text directly in screen space (same API as FPS overlay)
  if (dialogue_font_) {
    Engine::Graphics::draw_text_wrapped_ui(
      dialogue_font_,
      OLD_MAN_DIALOGUE_TEXT,
      Math::Vec2D(185.0f, 615.0f),
      1.0f,
      Math::Vec4D(0.0f, 0.0f, 0.0f, 1.0f),
      880.0f,
      1.15f,
      0  // Left alignment
    );
  }

  if (continue_font_) {
    Engine::Graphics::draw_text_wrapped_ui(
      continue_font_,
      "Press any key to continue",
      Math::Vec2D(400.0f, 825.0f),
      1.0f,
      Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f),
      800.0f,
      1.0f,
      1  // Center alignment
    );
  }
}

void OldManDialogueState::free() {}

void OldManDialogueState::unload()
{
  if (dimmer_id_ != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(dimmer_id_))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(dimmer_id_));
  }
  if (dialogue_bg_id_ != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(dialogue_bg_id_))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(dialogue_bg_id_));
  }
  if (npc_portrait_id_ != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(npc_portrait_id_))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(npc_portrait_id_));
  }
  dimmer_id_ = 0;
  dialogue_bg_id_ = 0;
  npc_portrait_id_ = 0;
  dialogue_font_ = nullptr;
  continue_font_ = nullptr;

  is_open = false;
  Engine::Time::set_time_scale(1.0f);
}