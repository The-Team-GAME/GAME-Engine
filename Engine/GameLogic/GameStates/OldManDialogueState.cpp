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

void OldManDialogueState::load()
{
  Log::info("[OldManFlow] Dialogue load");
  is_open = true;
  open_tips_request = false;
  close_enable_time_unscaled_ = Engine::Time::get_unscaled_time() + 0.12f;
  e_close_armed_ = false;
  request_tips_on_close_ = false;

  dialogue_font_ = Engine::Resources::load_font("fonts/FiveFontsatFreddy's-Regular.ttf", 48);
  continue_font_ = Engine::Resources::load_font("fonts/FiveFontsatFreddy's-Regular.ttf", 32);

  auto dialogue_bg_tex = Engine::Resources::load_texture("images/NotBroDialogue.png");
  if (!dialogue_bg_tex) {
    dialogue_bg_tex = Engine::Resources::get_rect_texture();
  }

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
    .add_component<Components::Texture>(dialogue_bg_tex)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_5")
    .get_id();

  auto portrait_tex = Engine::Resources::load_texture("images/NPCOldman.png");
  if (!portrait_tex) {
    portrait_tex = Engine::Resources::load_texture("images/OldManJorkin_Jorkin_Spritesheet.png");
  }

  npc_portrait_id_ = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(SCREEN_CENTER_X + 509.0f, SCREEN_CENTER_Y + 173.0f, 0.0f, 320.0f, 320.0f)
    .add_component<Components::Texture>(portrait_tex)
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_6")
    .get_id();

  // Freeze world updates while dialogue scene is on top; dialogue uses raw/unscaled input.
  Engine::Time::set_time_scale(0.0001f);
}

void OldManDialogueState::init() {}

void OldManDialogueState::update()
{
  const bool esc_pressed = Engine::Input::is_pressed_raw(Engine::KeyCode::Escape) || Engine::Input::is_pressed(Engine::KeyCode::Escape);
  if (esc_pressed) {
    Log::info("[OldManFlow] Dialogue close by ESC");
    request_tips_on_close_ = false;
    block_oldman_reopen_until_unscaled = Engine::Time::get_unscaled_time() + 1.0f;
    Engine::SceneManager::pop_scene();
    return;
  }

  const bool e_down = Engine::Input::is_held_raw(Engine::KeyCode::E) || Engine::Input::is_held(Engine::KeyCode::E);
  if (!e_close_armed_ && !e_down) {
    e_close_armed_ = true;
    return;
  }

  const bool e_pressed = Engine::Input::is_pressed_raw(Engine::KeyCode::E) || Engine::Input::is_pressed(Engine::KeyCode::E);
  if (e_close_armed_ && e_pressed && Engine::Time::get_unscaled_time() >= close_enable_time_unscaled_) {
    Log::info("[OldManFlow] Dialogue close by E (request tips)");
    request_tips_on_close_ = true;
    suppress_oldman_reopen_once = true;
    block_oldman_reopen_until_unscaled = Engine::Time::get_unscaled_time() + 1.0f;
    Engine::SceneManager::pop_scene();
    return;
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
      "Press E to continue",
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
  Log::info("[OldManFlow] Dialogue unload (request_tips_on_close_={})", request_tips_on_close_ ? 1 : 0);
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
  open_tips_request = open_tips_request || request_tips_on_close_;
  Log::info("[OldManFlow] open_tips_request now {}", open_tips_request ? 1 : 0);
  request_tips_on_close_ = false;
  Engine::Time::set_time_scale(1.0f);
}