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
#include "../../Engine/Engine/Graphics/Renderer.h"

namespace {
  Engine::ECS::Entity find_tips_button_entity_for_overlay() {
    Engine::ECS::Entity found;
    Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
      if (found.is_valid()) return;
      if (entity.get_name() == "TipsButtonHUD") {
        found = entity;
      }
      });

    if (found.is_valid()) return found;

    Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
      if (found.is_valid()) return;
      if (!entity.has_component<Components::UI>()) return;
      auto* tex = entity.get_component<Components::Texture>();
      if (!tex || !tex->texture_handle) return;
      if (Engine::Resources::get_relative_path(tex->texture_handle->path) == "images/UI/TipsBook.png") {
        found = entity;
      }
      });

    return found;
  }
}

void TipsBookState::ensure_nav_button()
{
  if (tips_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) {
    owns_nav_button_ = false;
    return;
  }

  auto shared_btn = find_tips_button_entity_for_overlay();
  if (shared_btn.is_valid()) {
    tips_button_id = shared_btn.get_id();
    owns_nav_button_ = false;
    return;
  }

  auto temp_btn = Engine::ECS::create_entity()
    .set_name("TipsOverlayNavButton")
    .add_component<Components::Transform2D>(TIPS_OVERLAY_CENTER_X, TIPS_OVERLAY_CENTER_Y, 0.0f, TIPS_BUTTON_NAV_SIZE, TIPS_BUTTON_NAV_SIZE)
    .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/arrowright.png"))
    .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    .add_component<Components::UI>()
    .add_component<Components::RuntimeOnly>()
    .add_to_layer("overlay_items_3");
  tips_button_id = temp_btn.get_id();
  owns_nav_button_ = true;
}

// ============================================================
// Nav button helpers
// ============================================================

void TipsBookState::set_nav_right()
{
  if (tips_button_id == 0) return;
  Engine::ECS::Entity btn(tips_button_id);
  if (!btn.is_valid()) return;

  btn.clear_layers().add_to_layer("overlay_items_3");

  float half_w = TIPS_OVERLAY_WIDTH * 0.5f;
  float half_h = TIPS_OVERLAY_HEIGHT * 0.5f;
  float button_half = TIPS_BUTTON_NAV_SIZE * 0.5f;

  float screen_x = TIPS_OVERLAY_CENTER_X + half_w - TIPS_NAV_MARGIN_X - button_half;
  float screen_y = TIPS_OVERLAY_CENTER_Y + half_h - TIPS_NAV_MARGIN_Y - button_half + TIPS_NAV_OFFSET_Y;

  if (auto* t = btn.get_component<Components::Transform2D>()) {
    float anchor_x = 0.0f;
    float anchor_y = 0.0f;
    if (auto* ui = btn.get_component<Components::UI>()) {
      anchor_x = ui->anchor.x;
      anchor_y = ui->anchor.y;
    }
    auto vres = Engine::Graphics::get_virtual_resolution();
    t->position.x = screen_x - anchor_x * vres.x;
    t->position.y = screen_y - anchor_y * vres.y;
    t->scale.x = TIPS_BUTTON_NAV_SIZE;
    t->scale.y = TIPS_BUTTON_NAV_SIZE;
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

  btn.clear_layers().add_to_layer("overlay_items_3");

  float half_w = TIPS_OVERLAY_WIDTH * 0.5f;
  float half_h = TIPS_OVERLAY_HEIGHT * 0.5f;
  float button_half = TIPS_BUTTON_NAV_SIZE * 0.5f;

  float screen_x = TIPS_OVERLAY_CENTER_X - half_w + TIPS_NAV_MARGIN_X + button_half;
  float screen_y = TIPS_OVERLAY_CENTER_Y + half_h - TIPS_NAV_MARGIN_Y - button_half + TIPS_NAV_OFFSET_Y;

  if (auto* t = btn.get_component<Components::Transform2D>()) {
    float anchor_x = 0.0f;
    float anchor_y = 0.0f;
    if (auto* ui = btn.get_component<Components::UI>()) {
      anchor_x = ui->anchor.x;
      anchor_y = ui->anchor.y;
    }
    auto vres = Engine::Graphics::get_virtual_resolution();
    t->position.x = screen_x - anchor_x * vres.x;
    t->position.y = screen_y - anchor_y * vres.y;
    t->scale.x = TIPS_BUTTON_NAV_SIZE;
    t->scale.y = TIPS_BUTTON_NAV_SIZE;
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
    t->scale.x = TIPS_BUTTON_HUD_SIZE;
    t->scale.y = TIPS_BUTTON_HUD_SIZE;
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
    .add_to_layer("overlay_items_1")
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
    .add_to_layer("overlay_items_1")
    .get_id();

  set_nav_left();
}

// ============================================================
// Scene lifecycle
// ============================================================

void TipsBookState::load()
{
  is_open = true;
  ensure_nav_button();

  bg_id = Engine::UI::create_screen_dimmer(0.5f, "overlay_bg", true);

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

  // Any-key navigation: advance page or close when on last page.
  if (Engine::Input::is_any_key_pressed()) {
    if (page == 1) {
      show_page2();
    } else {
      Engine::SceneManager::pop_scene();
    }
    return;
  }

  // Mouse click on nav arrow also toggles page.
  if (tips_button_id == 0 || !Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id)))
    return;

  glm::vec2 mouse_pos = Engine::Input::get_mouse_position();
  auto* t = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
  auto* ui = Engine::ECS::Entity(tips_button_id).get_component<Components::UI>();
  if (!t || !ui) return;

  auto vres = Engine::Graphics::get_virtual_resolution();
  float sx = vres.x * ui->anchor.x + t->position.x;
  float sy = vres.y * ui->anchor.y + t->position.y;
  float hw = t->scale.x * 0.5f;
  float hh = t->scale.y * 0.5f;
  bool  hovered = (mouse_pos.x >= sx - hw && mouse_pos.x <= sx + hw &&
    mouse_pos.y >= sy - hh && mouse_pos.y <= sy + hh);

  if (hovered && Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
    if (page == 1) show_page2();
    else           show_page1();
  }
}

void TipsBookState::draw() {}
void TipsBookState::free() {}

void TipsBookState::unload()
{
  is_open = false;
  if (bg_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(bg_id)))
    Engine::ECS::destroy_entity(Engine::ECS::Entity(bg_id));
  if (overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(overlay_id)))
    Engine::ECS::destroy_entity(Engine::ECS::Entity(overlay_id));
  bg_id = 0;
  overlay_id = 0;
  page = 1;

  if (owns_nav_button_) {
    if (tips_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_button_id));
    }
    tips_button_id = 0;
    owns_nav_button_ = false;
  }
  else {
    restore_hud_button();
  }
  Engine::Time::set_time_scale(1.0f);
}
