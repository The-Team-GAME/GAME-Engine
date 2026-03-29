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

void OldManDialogueState::load()
{
  Log::info("[OldManFlow] Dialogue load");
  is_open = true;
  open_tips_request = false;
  current_slide_ = 0;
  close_enable_time_unscaled_ = Engine::Time::get_unscaled_time() + 0.12f;
  e_close_armed_ = false;
  request_tips_on_close_ = false;

  dialogue_sprite_id_ = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(SLIDE_X, SLIDE_Y, 0.0f, SLIDE_W * SLIDE_SCALE, SLIDE_H * SLIDE_SCALE)
    .add_component<Components::Texture>(Engine::Resources::load_texture(SLIDE_PATHS[0]))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("overlay_items_1")
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

  // Wait for E (the key that opened the dialogue) to be released before arming.
  const bool e_down = Engine::Input::is_held_raw(Engine::KeyCode::E) || Engine::Input::is_held(Engine::KeyCode::E);
  if (!e_close_armed_ && !e_down) {
    e_close_armed_ = true;
    return;
  }

  if (e_close_armed_ && Engine::Input::is_any_key_pressed() && Engine::Time::get_unscaled_time() >= close_enable_time_unscaled_) {
    if (current_slide_ < SLIDE_COUNT - 1) {
      // Advance to next slide
      ++current_slide_;
      auto* tex = Engine::ECS::Entity(dialogue_sprite_id_).get_component<Components::Texture>();
      if (tex) {
        tex->texture_handle = Engine::Resources::load_texture(SLIDE_PATHS[current_slide_]);
      }
      Log::info("[OldManFlow] Advanced to slide {}", current_slide_ + 1);
    } else {
      // Last slide — close dialogue and request tips
      Log::info("[OldManFlow] Dialogue close by key press (request tips)");
      request_tips_on_close_ = true;
      suppress_oldman_reopen_once = true;
      block_oldman_reopen_until_unscaled = Engine::Time::get_unscaled_time() + 1.0f;
      Engine::SceneManager::pop_scene();
    }
  }
}

void OldManDialogueState::draw()
{
  // All dialogue content is baked into the slide sprites — nothing to draw here.
}

void OldManDialogueState::free() {}

void OldManDialogueState::unload()
{
  Log::info("[OldManFlow] Dialogue unload (request_tips_on_close_={})", request_tips_on_close_ ? 1 : 0);
  if (dialogue_sprite_id_ != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(dialogue_sprite_id_))) {
    Engine::ECS::destroy_entity(Engine::ECS::Entity(dialogue_sprite_id_));
  }
  dialogue_sprite_id_ = 0;

  is_open = false;
  open_tips_request = open_tips_request || request_tips_on_close_;
  Log::info("[OldManFlow] open_tips_request now {}", open_tips_request ? 1 : 0);
  request_tips_on_close_ = false;
  Engine::Time::set_time_scale(1.0f);
}