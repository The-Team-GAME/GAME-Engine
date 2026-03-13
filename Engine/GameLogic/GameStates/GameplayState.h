//-----------------------------------------------------------------------------------
// file:    GameplayState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (70%)
//          Ryan Ang An Kiat (30%)
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

#include "pch.h"

#include "../Components/GameplayComponent.h"
#include "../Components/InteractableComponents.h"
#include "../Components/HealthComponents.h"
#include "../../Engine/UpgradeManager.h"
#include "../../Engine/UpgradeUIStateMachine.h"
#include "../../Engine/TempUpgradeUIManager.h"


class GameplayState : public Engine::Scene
{
  static constexpr const char* GAMEPLAY_BGM_EVENT = "event:/BGM/LevelBGM";
  static constexpr const char* GAMEPLAY_BATTLECRY_EVENT = "event:/SFX/PlayerStartCry";
  static constexpr float TIPS_BUTTON_HUD_X = 1373.0f;
  static constexpr float TIPS_BUTTON_HUD_Y = 92.0f;
  static constexpr float TIPS_BUTTON_SIZE = 220.0f;
  static constexpr float TIPS_OVERLAY_CENTER_X = 800.0f;
  static constexpr float TIPS_OVERLAY_CENTER_Y = 450.0f;
  static constexpr float TIPS_OVERLAY_WIDTH = 1088.0f;
  static constexpr float TIPS_OVERLAY_HEIGHT = 768.0f;
  static constexpr float TIPS_NAV_MARGIN_X = 24.0f;
  static constexpr float TIPS_NAV_MARGIN_Y = 24.0f;
  // UI entity IDs
  Engine::ECS::EntityID pause_button_id = 0;
  Engine::ECS::EntityID tips_button_id = 0;
  Engine::ECS::EntityID tips_overlay_id = 0;
  Engine::ECS::EntityID tips_overlay_page2_id = 0;
  Engine::ECS::EntityID tips_bg_id = 0;
  Engine::ECS::EntityID tips_next_btn_id = 0;
  Engine::ECS::EntityID upgrade_btn_id = 0;
  bool tips_shown = false;
  int tips_page = 1;

  Engine::ECS::Entity find_tips_button_entity() {
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

public:
  void load() {
    Engine::Resources::open_level("levels/LevelOneTestingExtra.json");


    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    // TODO: REMOVE 
    player_entity.add_component<Components::Health>(5, 5);

    // Fix camera follow: parent the camera entity to the player so CameraSystem tracks it
    if (player_entity.is_valid()) {
      auto camera_entity = Engine::ECS::get_first_entity_with<Components::Camera>();
      if (camera_entity.is_valid()) {
        if (!camera_entity.has_component<Components::Parent>()) {
          camera_entity.add_component<Components::Parent>(player_entity.get_id());
        }
        else {
          auto* parent_comp = camera_entity.get_component<Components::Parent>();
          if (parent_comp) {
            parent_comp->parent_id = player_entity.get_id();
            parent_comp->offset_initialized = false; // Let HierarchySystem recalculate
          }
        }
      }
    }
    // player_entity.add_component<Components::HeartHUDSprite>();

    // Victory zone - triggers victory screen when player enters
    // TODO: Position this at the end of the level
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(14614.0f, 609.0f, 0.0f, 100.0f, 200.0f)
      .add_component<Components::Interactable>(50.0f, "show_victory", "", "")
      .add_to_layer("effect");

    // // Old man NPC - press E to interact and show dialogue
    // auto old_man = Engine::ECS::create_entity()
    //   .add_component<Components::Transform2D>(200.0f, 800.0f, 0.0f, 64.0f, 64.0f)
    //   .add_component<Components::Texture>(Engine::Resources::load_texture("images/NPCOldman.png"))
    //   .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
    //   .add_component<Components::Interactable>(100.0f, "", "old_man_on_update", "old_man_on_exit")
    //   .add_to_layer("player");
    // Log::info("Old man entity created with ID: {}", old_man.get_id());

    // Create UI buttons - use UI component for screen-space rendering
    // Positions are in screen coordinates (0,0 is top-left, 1600x900 viewport)
    // Pause button - top right (screen coords: 1600 - padding - half_width)
    auto pause_btn = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(1500.0f, 90.0f, 0.0f, 100.0f, 100.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Pause.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    pause_button_id = pause_btn.get_id();

    // Tips book button - top right beside pause (reuse level entity if present)
    auto tips_btn = find_tips_button_entity();
    if (tips_btn.is_valid()) {
      tips_button_id = tips_btn.get_id();
      set_tips_button_layer("main_menu_4");
      set_tips_button_hud_position();
    }
    else {
      auto tips_btn_created = Engine::ECS::create_entity()
        .add_component<Components::Transform2D>(TIPS_BUTTON_HUD_X, TIPS_BUTTON_HUD_Y, 0.0f, TIPS_BUTTON_SIZE, TIPS_BUTTON_SIZE)
        .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/TipsBook.png"))
        .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
        .add_component<Components::UI>()
        .add_to_layer("main_menu_4");
      tips_button_id = tips_btn_created.get_id();
    }

    // Upgrade button — bottom-right corner, 64x64, alpha-hidden until upgrade is available
    auto upg_btn = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(1500.0f, 800.0f, 0.0f, 64.0f * 4.0f, 64.0f * 4.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/skill_upgrades.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)  // start invisible
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    upgrade_btn_id = upg_btn.get_id();
    Engine::UpgradeUIStateMachine::instance().set_upgrade_button(upgrade_btn_id);

  };

  void init() {
    Engine::AudioSystem::playSceneMusic(GAMEPLAY_BGM_EVENT, 0.3f, true); // TODO: get from settings
    Engine::AudioSystem::playEvent(GAMEPLAY_BATTLECRY_EVENT); // TODO: get from settings
  };

  void update() {
    // UI elements use screen-space coordinates (no camera tracking needed)
    // Get mouse position in screen/viewport coordinates for UI click detection
    glm::vec2 mouse_pos = Engine::Input::get_mouse_position();

    if (tips_shown) {
      if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
        destroy_tips_overlay();
        return;
      }

      if (tips_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) {
        auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
        float half_w = tips_transform->scale.x / 2.0f;
        float half_h = tips_transform->scale.y / 2.0f;

        if (mouse_pos.x >= tips_transform->position.x - half_w &&
          mouse_pos.x <= tips_transform->position.x + half_w &&
          mouse_pos.y >= tips_transform->position.y - half_h &&
          mouse_pos.y <= tips_transform->position.y + half_h &&
          Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
          if (tips_page == 1) {
            show_tips_page2();
          }
          else if (tips_page == 2) {
            show_tips_page1();
          }
        }
      }

      return;
    }

    // Pause button click detection (screen coords)
    if (pause_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(pause_button_id))) {
      auto* pause_transform = Engine::ECS::Entity(pause_button_id).get_component<Components::Transform2D>();
      float half_w = pause_transform->scale.x / 2.0f;
      float half_h = pause_transform->scale.y / 2.0f;

      if (mouse_pos.x >= pause_transform->position.x - half_w &&
        mouse_pos.x <= pause_transform->position.x + half_w &&
        mouse_pos.y >= pause_transform->position.y - half_h &&
        mouse_pos.y <= pause_transform->position.y + half_h) {
        if (Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
          // Close tips if open, then pause
          if (tips_shown) {
            destroy_tips_overlay();
          }
          Engine::SceneManager::push_scene("PauseState");
          return;
        }
      }
    }

    // Tips button click detection (screen coords)
    if (!tips_shown && tips_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) {
      auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
      float half_w = tips_transform->scale.x / 2.0f;
      float half_h = tips_transform->scale.y / 2.0f;

      if (mouse_pos.x >= tips_transform->position.x - half_w &&
        mouse_pos.x <= tips_transform->position.x + half_w &&
        mouse_pos.y >= tips_transform->position.y - half_h &&
        mouse_pos.y <= tips_transform->position.y + half_h) {
        if (Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
          show_tips_overlay();
          return;
        }
      }
    }

    // Check if pause button is pressed (keyboard)
    if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
      Engine::SceneManager::push_scene("PauseState");
      return;
    }

    Engine::UpgradeUIStateMachine::instance().update();

  };

  bool is_tips_overlay_active() const {
    if (tips_bg_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_bg_id))) return true;
    if (tips_overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_id))) return true;
    if (tips_overlay_page2_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_page2_id))) return true;
    if (tips_next_btn_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_next_btn_id))) return true;
    return false;
  }

  void set_tips_button_position(float x, float y) {
    if (tips_button_id == 0 || !Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) return;
    auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
    if (!tips_transform) return;
    tips_transform->position.x = x;
    tips_transform->position.y = y;
  }

  void set_tips_button_layer(const char* layer_name) {
    if (tips_button_id == 0 || !Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) return;
    Engine::ECS::Entity tips_entity(tips_button_id);
    tips_entity.clear_layers().add_to_layer(layer_name);
  }

  void set_tips_button_texture(const char* texture_path) {
    if (tips_button_id == 0 || !Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) return;
    Engine::ECS::Entity tips_entity(tips_button_id);
    if (tips_entity.has_component<Components::TextureAtlas>()) {
      tips_entity.remove_component<Components::TextureAtlas>();
    }
    auto* tips_texture = tips_entity.get_component<Components::Texture>();
    if (!tips_texture) return;
    *tips_texture = Components::Texture(Engine::Resources::load_texture(texture_path));
  }

  void set_tips_button_color(float r, float g, float b, float a) {
    if (tips_button_id == 0 || !Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) return;
    auto* tips_color = Engine::ECS::Entity(tips_button_id).get_component<Components::Color>();
    if (!tips_color) return;
    *tips_color = Components::Color(r, g, b, a);
  }

  void set_tips_button_hud_position() {
    set_tips_button_position(TIPS_BUTTON_HUD_X, TIPS_BUTTON_HUD_Y);
  }

  void set_tips_button_overlay_position(bool right_side) {
    float half_w = TIPS_OVERLAY_WIDTH * 0.5f;
    float half_h = TIPS_OVERLAY_HEIGHT * 0.5f;
    float button_half = TIPS_BUTTON_SIZE * 0.5f;

    float x = right_side
      ? (TIPS_OVERLAY_CENTER_X + half_w - TIPS_NAV_MARGIN_X - button_half)
      : (TIPS_OVERLAY_CENTER_X - half_w + TIPS_NAV_MARGIN_X + button_half);
    float y = TIPS_OVERLAY_CENTER_Y + half_h - TIPS_NAV_MARGIN_Y - button_half;

    set_tips_button_position(x, y);
  }

  void show_tips_overlay() {
    if (tips_shown) {
      if (is_tips_overlay_active()) return;
      // Reset stale state so the overlay can re-open after ECS cleanup.
      destroy_tips_overlay();
    }
    tips_shown = true;
    tips_page = 1;

    // Create dark background overlay (semi-transparent black)
    auto bg = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.5f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    tips_bg_id = bg.get_id();

    // Create tips overlay page 1 (screen-space coords)
    auto overlay = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1088.0f, 768.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/InstructionsTips.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    tips_overlay_id = overlay.get_id();

    set_tips_button_layer("main_menu_6");
    set_tips_button_overlay_position(true);
    set_tips_button_texture("images/UI/arrowright.png");
    set_tips_button_color(1.0f, 1.0f, 1.0f, 1.0f);

    // Use the tips button near the pause button to advance pages.
    tips_next_btn_id = 0;

    // Pause the game while tips are showing
    Engine::Time::set_time_scale(0.0001f);
  }

  void show_tips_page2() {
    tips_page = 2;

    // Destroy page 1 overlay and next button
    if (tips_overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_overlay_id));
    }
    if (tips_next_btn_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_next_btn_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_next_btn_id));
    }
    tips_overlay_id = 0;
    tips_next_btn_id = 0;

    // Create tips overlay page 2 (screen-space coords)
    auto overlay2 = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1088.0f, 768.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/InstructionsTips2.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    tips_overlay_page2_id = overlay2.get_id();

    set_tips_button_layer("main_menu_6");
    set_tips_button_overlay_position(false);
    set_tips_button_texture("images/UI/arrowleft.png");
    set_tips_button_color(1.0f, 1.0f, 1.0f, 1.0f);
  }

  void show_tips_page1() {
    tips_page = 1;

    if (tips_overlay_page2_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_page2_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_overlay_page2_id));
    }
    if (tips_overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_overlay_id));
    }
    tips_overlay_page2_id = 0;
    tips_overlay_id = 0;

    auto overlay = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1088.0f, 768.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/InstructionsTips.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    tips_overlay_id = overlay.get_id();

    set_tips_button_layer("main_menu_6");
    set_tips_button_overlay_position(true);
    set_tips_button_texture("images/UI/arrowright.png");
    set_tips_button_color(1.0f, 1.0f, 1.0f, 1.0f);
  }

  void destroy_tips_overlay() {
    if (!tips_shown) return;

    if (tips_bg_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_bg_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_bg_id));
    }
    if (tips_overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_overlay_id));
    }
    if (tips_overlay_page2_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_page2_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_overlay_page2_id));
    }
    if (tips_next_btn_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_next_btn_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_next_btn_id));
    }
    tips_bg_id = 0;
    tips_overlay_id = 0;
    tips_overlay_page2_id = 0;
    tips_next_btn_id = 0;
    tips_shown = false;
    tips_page = 1;

    set_tips_button_layer("main_menu_4");
    set_tips_button_hud_position();
    set_tips_button_texture("images/UI/TipsBook.png");
    set_tips_button_color(1.0f, 1.0f, 1.0f, 1.0f);

    // Resume the game
    Engine::Time::set_time_scale(1.0f);
  }

  void draw() {
    // Delegates to Engine EXE side — keeps all ImGui calls out of the DLL.
    Engine::TempUpgradeUIManager::draw();
  };

  void free() {
    Engine::UpgradeUIStateMachine::instance().reset();

    // Clean up tips overlay if still shown
    if (tips_shown) {
      destroy_tips_overlay();
    }
  };

  void unload() {
    Engine::AudioSystem::stopSceneMusic(true);
  };
};