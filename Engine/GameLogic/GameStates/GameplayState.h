//-----------------------------------------------------------------------------------
// file:    GameplayState.h
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
#pragma once

/* EVERY CODE HERE IS TEMPORARY */

#include "pch.h"

#include "../Components/GameplayComponent.h"
#include "../Components/InteractableComponents.h"
#include "../Components/HealthComponents.h"
#include "../../Engine/UpgradeManager.h"
#include "../../Engine/TempUpgradeUIManager.h"


class GameplayState : public Engine::Scene
{
  Engine::Audio::Sound bgm_sound;
  Engine::Audio::ChannelHandle bgm_channel = Engine::Audio::INVALID_CHANNEL;
  // UI entity IDs
  Engine::ECS::EntityID pause_button_id = 0;
  Engine::ECS::EntityID tips_button_id = 0;
  Engine::ECS::EntityID tips_overlay_id = 0;
  Engine::ECS::EntityID tips_bg_id = 0;
  Engine::ECS::EntityID upgrade_btn_id = 0;
  bool tips_shown = false;

public:
  void load() {
    Engine::Resources::open_level("levels/LevelOneTestingExtra.json");

    bgm_sound = Engine::Resources::load_sound("sounds/BGM/Level 1/MonomythBGM.wav");


    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    // Camera entity
    // Engine::ECS::Entity camera = Engine::ECS::create_entity();
    // camera.add_component<Components::Transform2D>(-3239.0f, 471.58f, 0.0f, 1.0f, 1.0f);
    // camera.add_component<Components::Camera>("player camera", true);
    // camera.add_component<Components::Parent>(player_entity.get_id());

    player_entity.add_component<Components::Health>(5, 5);
    player_entity.add_component<Components::HeartHUDSprite>();

    // Victory zone - triggers victory screen when player enters
    // TODO: Position this at the end of the level
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(14600.0f, 830.0f, 0.0f, 100.0f, 200.0f)
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
      .add_component<Components::Transform2D>(1520.0f, 80.0f, 0.0f, 126.0f, 131.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Pause.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    pause_button_id = pause_btn.get_id();

    // Tips book button - to the left of pause button
    auto tips_btn = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(1373.0f, 92.0f, 0.0f, 110.0f, 110.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/TipsBook.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    tips_button_id = tips_btn.get_id();

    // Upgrade button — bottom-right corner, 64x64, alpha-hidden until upgrade is available
    auto upg_btn = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(1552.0f, 852.0f, 0.0f, 64.0f * 2.0f, 64.0f * 2.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/Upgrade.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)  // start invisible
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    upgrade_btn_id = upg_btn.get_id();

  };

  void init() {
    bgm_channel = Engine::Audio::play_sound(bgm_sound, 0.3f, true); // TODO: get from settings
  };

  void update() {
    // UI elements use screen-space coordinates (no camera tracking needed)
    // Get mouse position in screen/viewport coordinates for UI click detection
    glm::vec2 mouse_pos = Engine::Input::get_mouse_position();

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
    if (tips_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) {
      auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
      float half_w = tips_transform->scale.x / 2.0f;
      float half_h = tips_transform->scale.y / 2.0f;

      if (mouse_pos.x >= tips_transform->position.x - half_w &&
        mouse_pos.x <= tips_transform->position.x + half_w &&
        mouse_pos.y >= tips_transform->position.y - half_h &&
        mouse_pos.y <= tips_transform->position.y + half_h) {
        if (Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
          if (!tips_shown) {
            show_tips_overlay();
          }
        }
      }
    }

    // Close tips overlay when clicking anywhere (if shown)
    if (tips_shown && Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
      // Check if NOT clicking on tips button (to avoid immediate close after open)
      auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
      float half_w = tips_transform->scale.x / 2.0f;
      float half_h = tips_transform->scale.y / 2.0f;

      bool clicking_tips_btn = (mouse_pos.x >= tips_transform->position.x - half_w &&
        mouse_pos.x <= tips_transform->position.x + half_w &&
        mouse_pos.y >= tips_transform->position.y - half_h &&
        mouse_pos.y <= tips_transform->position.y + half_h);

      if (!clicking_tips_btn) {
        destroy_tips_overlay();
      }
    }

    // Check if pause button is pressed (keyboard)
    if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
      if (tips_shown) {
        destroy_tips_overlay();
      }
      else {
        Engine::SceneManager::push_scene("PauseState");
      }
    }

    // Upgrade button — show/hide based on upgrade availability, click to open picker
    auto& upg_mgr = Engine::UpgradeManager::instance();
    if (upgrade_btn_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(upgrade_btn_id))) {
      auto* upg_color = Engine::ECS::Entity(upgrade_btn_id).get_component<Components::Color>();
      if (upg_color)
        upg_color->a = upg_mgr.can_upgrade() ? 1.0f : 0.0f;

      if (upg_mgr.can_upgrade()) {
        auto* upg_t = Engine::ECS::Entity(upgrade_btn_id).get_component<Components::Transform2D>();
        float uhw = upg_t->scale.x / 2.0f;
        float uhh = upg_t->scale.y / 2.0f;
        if (mouse_pos.x >= upg_t->position.x - uhw &&
          mouse_pos.x <= upg_t->position.x + uhw &&
          mouse_pos.y >= upg_t->position.y - uhh &&
          mouse_pos.y <= upg_t->position.y + uhh) {
          if (Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
            if (!upg_mgr.has_rolled_cards())
              upg_mgr.roll_cards();
            upg_mgr.request_upgrade_picker();
          }
        }
      }
    }

  };

  void show_tips_overlay() {
    if (tips_shown) return;
    tips_shown = true;

    // Create dark background overlay (semi-transparent black)
    auto bg = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.5f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    tips_bg_id = bg.get_id();

    // Create tips overlay (screen-space coords)
    auto overlay = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1088.0f, 768.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/InstructionsTips.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    tips_overlay_id = overlay.get_id();

    // Pause the game while tips are showing
    Engine::Time::set_time_scale(0.0001f);
  }

  void destroy_tips_overlay() {
    if (!tips_shown) return;

    if (tips_bg_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_bg_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_bg_id));
    }
    if (tips_overlay_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_overlay_id))) {
      Engine::ECS::destroy_entity(Engine::ECS::Entity(tips_overlay_id));
    }
    tips_bg_id = 0;
    tips_overlay_id = 0;
    tips_shown = false;

    // Resume the game
    Engine::Time::set_time_scale(1.0f);
  }

  void draw() {
    // Delegates to Engine EXE side — keeps all ImGui calls out of the DLL.
    Engine::TempUpgradeUIManager::draw();
  };

  void free() {
    // Clean up tips overlay if still shown
    if (tips_shown) {
      destroy_tips_overlay();
    }
  };

  void unload() {
    Engine::Audio::stop_all_sounds();
  };
};