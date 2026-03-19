//-----------------------------------------------------------------------------------
// file:    TipsBookState.cpp
// project: Monkey Stole My GPA
//
// brief:   See TipsBookState.h
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "TipsBookState.h"

// ============================================================
// Nav button helpers
// ============================================================

void TipsBookState::set_nav_right()
{
  if (tips_button_id == 0) return;
  Engine::ECS::Entity btn(tips_button_id);
  if (!btn.is_valid()) return;

  btn.clear_layers().add_to_layer("main_menu_6");

  float half_w      = TIPS_OVERLAY_WIDTH  * 0.5f;
  float half_h      = TIPS_OVERLAY_HEIGHT * 0.5f;
  float button_half = TIPS_BUTTON_SIZE    * 0.5f;

  float x = TIPS_OVERLAY_CENTER_X + half_w - TIPS_NAV_MARGIN_X - button_half;
  float y = TIPS_OVERLAY_CENTER_Y + half_h - TIPS_NAV_MARGIN_Y - button_half;
  x -= 1600.0f; // anchor.x = 1.0 => final_screen_x = 1600 + position.x

  if (auto* t = btn.get_component<Components::Transform2D>()) {
    t->position.x = x;
    t->position.y = y;
  }
  if (btn.has_component<Components::TextureAtlas>())
    btn.remove_component<Components::TextureAtlas>();
  if (auto* tex = btn.get_component<Components::Texture>())
    *tex = Components::Texture(Engine::Resources::load_texture("images/UI/arrowright.png"));
  if (auto* col = btn.get_component<Components::Color>())
    *col = Components::Color(1.0f, 1.0f, 1.0f, 1.0f);
}

void TipsBookState::set_nav_left()
{
  if (tips_button_id == 0) return;
  Engine::ECS::Entity btn(tips_button_id);
  if (!btn.is_valid()) return;

  btn.clear_layers().add_to_layer("main_menu_6");

  float half_w      = TIPS_OVERLAY_WIDTH  * 0.5f;
  float half_h      = TIPS_OVERLAY_HEIGHT * 0.5f;
  float button_half = TIPS_BUTTON_SIZE    * 0.5f;

  float x = TIPS_OVERLAY_CENTER_X - half_w + TIPS_NAV_MARGIN_X + button_half;
  float y = TIPS_OVERLAY_CENTER_Y + half_h - TIPS_NAV_MARGIN_Y - button_half;
  x -= 1600.0f;

  if (auto* t = btn.get_component<Components::Transform2D>()) {
    t->position.x = x;
    t->position.y = y;
  }
  if (btn.has_component<Components::TextureAtlas>())
    btn.remove_component<Components::TextureAtlas>();
  if (auto* tex = btn.get_component<Components::Texture>())
    *tex = Components::Texture(Engine::Resources::load_texture("images/UI/arrowleft.png"));
  if (auto* col = btn.get_component<Components::Color>())
    *col = Components::Color(1.0f, 1.0f, 1.0f, 1.0f);
}

void TipsBookState::restore_hud_button()
{
  if (tips_button_id == 0) return;
  Engine::ECS::Entity btn(tips_button_id);
  if (!btn.is_valid()) return;

  btn.clear_layers().add_to_layer("main_menu_4");

  if (auto* t = btn.get_component<Components::Transform2D>()) {
    t->position.x = TIPS_BUTTON_HUD_X;
    t->position.y = TIPS_BUTTON_HUD_Y;
    t->scale.x    = TIPS_BUTTON_SIZE;
    t->scale.y    = TIPS_BUTTON_SIZE;
  }
  if (btn.has_component<Components::TextureAtlas>())
    btn.remove_component<Components::TextureAtlas>();
  if (auto* tex = btn.get_component<Components::Texture>())
    *tex = Components::Texture(Engine::Resources::load_texture("images/UI/TipsBook.png"));
  if (auto* col = btn.get_component<Components::Color>())
    *col = Components::Color(1.0f, 1.0f, 1.0f, 1.0f);
}

// ============================================================
// Page switching
// ============================================================

void TipsBookState::show_page1()
{
  page = 1;
  if (overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(overlay_id)))
    Engine::ECS::destroy_entity(Engine::ECS::Entity(overlay_id));
  overlay_id = 0;

  overlay_id = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, TIPS_OVERLAY_WIDTH, TIPS_OVERLAY_HEIGHT)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/InstructionsTips.png"))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_5")
    .get_id();

  set_nav_right();
}

void TipsBookState::show_page2()
{
  page = 2;
  if (overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(overlay_id)))
    Engine::ECS::destroy_entity(Engine::ECS::Entity(overlay_id));
  overlay_id = 0;

  overlay_id = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, TIPS_OVERLAY_WIDTH, TIPS_OVERLAY_HEIGHT)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/InstructionsTips2.png"))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_5")
    .get_id();

  set_nav_left();
}

// ============================================================
// Scene lifecycle
// ============================================================

void TipsBookState::load()
{
  bg_id = Engine::ECS::create_entity()
    .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
    .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
    .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.5f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("main_menu_4")
    .get_id();

  show_page1();
  Engine::Time::set_time_scale(0.0001f);
}

void TipsBookState::init() {}

void TipsBookState::update()
{
  if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
    Engine::SceneManager::pop_scene();
    return;
  }

  if (tips_button_id == 0 || !Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id)))
    return;

  glm::vec2 mouse_pos = Engine::Input::get_mouse_position();
  auto* t = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
  if (!t) return;

  float hw = t->scale.x * 0.5f;
  float hh = t->scale.y * 0.5f;
  bool  hovered = (mouse_pos.x >= t->position.x - hw && mouse_pos.x <= t->position.x + hw &&
                   mouse_pos.y >= t->position.y - hh && mouse_pos.y <= t->position.y + hh);

  if (hovered && Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
    if (page == 1) show_page2();
    else           show_page1();
  }
}

void TipsBookState::draw() {}
void TipsBookState::free() {}

void TipsBookState::unload()
{
  if (bg_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(bg_id)))
    Engine::ECS::destroy_entity(Engine::ECS::Entity(bg_id));
  if (overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(overlay_id)))
    Engine::ECS::destroy_entity(Engine::ECS::Entity(overlay_id));
  bg_id      = 0;
  overlay_id = 0;
  page       = 1;

  restore_hud_button();
  Engine::Time::set_time_scale(1.0f);
}
