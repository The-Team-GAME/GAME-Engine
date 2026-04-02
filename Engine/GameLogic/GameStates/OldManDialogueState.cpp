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
#include "GameplayOverlayInputGuard.h"

void OldManDialogueState::destroy_dimmer()
{
  if (dimmer_id_ != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(dimmer_id_))) {
    Engine::ECS::Entity dimmer(dimmer_id_);
    dimmer.clear_layers();
    Engine::ECS::destroy_entity(dimmer);
  }
  dimmer_id_ = 0;
}

void OldManDialogueState::recreate_dimmer()
{
  destroy_dimmer();
  dimmer_id_ = Engine::UI::create_screen_dimmer(0.5f, "overlay_bg", true);
}

void OldManDialogueState::destroy_dialogue_sprite()
{
  if (dialogue_sprite_id_ != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(dialogue_sprite_id_))) {
    Engine::ECS::Entity dialogue_sprite(dialogue_sprite_id_);
    dialogue_sprite.clear_layers();
    Engine::ECS::destroy_entity(dialogue_sprite);
  }
  dialogue_sprite_id_ = 0;
}

void OldManDialogueState::recreate_dialogue_sprite()
{
  destroy_dialogue_sprite();

  dialogue_sprite_id_ = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(SLIDE_X, SLIDE_Y, 0.0f, SLIDE_W * SLIDE_SCALE, SLIDE_H * SLIDE_SCALE)
    .add_component<Components::Texture>(Engine::Resources::load_texture(SLIDE_PATHS[current_slide_]))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("overlay_items_1")
    .get_id();
}

void OldManDialogueState::load()
{
  GameplayOverlayInputGuard::acquire();
  Log::info("[OldManFlow] Dialogue load");
  is_open = true;
  open_tips_request = false;
  current_slide_ = 0;
  close_enable_time_unscaled_ = Engine::Time::get_unscaled_time() + 0.12f;
  e_close_armed_ = false;
  request_tips_on_close_ = false;
  tips_was_open_ = false;

  recreate_dimmer();
  recreate_dialogue_sprite();

  // Freeze world updates while dialogue scene is on top; dialogue uses raw/unscaled input.
  Engine::Time::set_time_scale(0.0001f);
}

void OldManDialogueState::init() {}

void OldManDialogueState::update()
{
  // Returning from TipsBookState: restore entities and re-freeze time.
  if (tips_was_open_) {
    tips_was_open_ = false;
    recreate_dimmer();
    recreate_dialogue_sprite();
    Engine::Time::set_time_scale(0.0001f);
    e_close_armed_ = false;
    close_enable_time_unscaled_ = Engine::Time::get_unscaled_time() + 0.30f;
    return;
  }

  const bool esc_pressed = Engine::Input::is_pressed_raw(Engine::KeyCode::Escape) || Engine::Input::is_pressed(Engine::KeyCode::Escape);
  if (esc_pressed) {
    Log::info("[OldManFlow] Dialogue close by ESC");
    request_tips_on_close_ = false;
    block_oldman_reopen_until_unscaled = Engine::Time::get_unscaled_time() + 1.0f;
    destroy_dialogue_sprite();
    destroy_dimmer();
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
      recreate_dialogue_sprite();
      Log::info("[OldManFlow] Advanced to slide {}", current_slide_ + 1);

      // After showing slide 4 (index 3), open tips book on top of dialogue
      if (current_slide_ == TIPS_PUSH_SLIDE) {
        TipsBookState::tips_button_id = 0; // no HUD button to return to
        Log::info("[OldManFlow] Pushing TipsBookState mid-dialogue");
        destroy_dimmer();
        destroy_dialogue_sprite();
        tips_was_open_ = true;
        Engine::SceneManager::push_scene("TipsBookState");
        // Re-arm handled in update() on return; no need to set here.
      }
    } else {
      // Last slide — close dialogue (tips was already shown mid-conversation)
      Log::info("[OldManFlow] Dialogue close by key press");
      suppress_oldman_reopen_once = true;
      block_oldman_reopen_until_unscaled = Engine::Time::get_unscaled_time() + 1.0f;
      destroy_dialogue_sprite();
      destroy_dimmer();
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
  destroy_dialogue_sprite();
  destroy_dimmer();

  is_open = false;
  open_tips_request = open_tips_request || request_tips_on_close_;
  Log::info("[OldManFlow] open_tips_request now {}", open_tips_request ? 1 : 0);
  request_tips_on_close_ = false;
  Engine::Time::set_time_scale(1.0f);
  GameplayOverlayInputGuard::release();
}