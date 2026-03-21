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

#include "TipsBookState.h"
#include "UpgradePickerState.h"
#include "OldManDialogueState.h"
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
  static constexpr float TIPS_BUTTON_HUD_X = -227.0f;
  static constexpr float TIPS_BUTTON_HUD_Y = 92.0f;
  static constexpr float TIPS_BUTTON_HUD_SIZE = 220.0f;
  // UI entity IDs
  Engine::ECS::EntityID pause_button_id = 0;
  Engine::ECS::EntityID tips_button_id = 0;
  Engine::ECS::EntityID upgrade_btn_id = 0;
  float oldman_tips_request_cooldown_until_ = 0.0f;

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
    // player_entity.add_component<Components::HeartHUDSprite>();

    // Victory zone - triggers victory screen when player enters
    // TODO: Position this at the end of the level
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(14600.0f, 830.0f, 0.0f, 100.0f, 200.0f)
      .add_component<Components::Interactable>(50.0f, "show_victory", "", "")
      .add_component<Components::RuntimeOnly>()
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
      .add_component<Components::Transform2D>(-100.0f, 90.0f, 0.0f, 100.0f, 100.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Pause.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>(1.0f, 0.0f)
      .add_component<Components::RuntimeOnly>()
      .add_to_layer("main_menu_4");
    pause_button_id = pause_btn.get_id();

    // Tips book button - top right beside pause (reuse level entity if present)
    auto tips_btn = find_tips_button_entity();
    if (tips_btn.is_valid()) {
      tips_button_id = tips_btn.get_id();
      set_tips_button_layer("main_menu_4");
      if (auto* ui = tips_btn.get_component<Components::UI>()) ui->anchor = { 1.0f, 0.0f };
      set_tips_button_size(TIPS_BUTTON_HUD_SIZE);
      set_tips_button_hud_position();
    }
    else {
      auto tips_btn_created = Engine::ECS::create_entity()
        .set_name("TipsButtonHUD")
        .add_component<Components::Transform2D>(TIPS_BUTTON_HUD_X, TIPS_BUTTON_HUD_Y, 0.0f, TIPS_BUTTON_HUD_SIZE, TIPS_BUTTON_HUD_SIZE)
        .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/TipsBook.png"))
        .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
        .add_component<Components::UI>(1.0f, 0.0f)
        .add_component<Components::RuntimeOnly>()
        .add_to_layer("main_menu_4");
      tips_button_id = tips_btn_created.get_id();
    }

    // Upgrade button — bottom-left corner, always visible; only clickable when upgrade is available
    auto upg_btn = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(128.0f, -128.0f, 0.0f, 64.0f * 4.0f, 64.0f * 4.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/skill_upgrades.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)  // always visible
      .add_component<Components::UI>(0.0f, 1.0f)
      .add_component<Components::RuntimeOnly>()
      .add_to_layer("main_menu_4");
    upgrade_btn_id = upg_btn.get_id();
    Engine::UpgradeUIStateMachine::instance().set_upgrade_button(upgrade_btn_id);
    Engine::UpgradeUIStateMachine::instance().set_picker_open_callback([](){
      Engine::SceneManager::push_scene("UpgradePickerState");
    });

  };

  void init() {
    Engine::AudioSystem::playSceneMusic(GAMEPLAY_BGM_EVENT, 0.3f, true); // TODO: get from settings
    Engine::AudioSystem::playEvent(GAMEPLAY_BATTLECRY_EVENT); // TODO: get from settings
  };

  void update() {
    const float now_unscaled = Engine::Time::get_unscaled_time();
    const bool tips_scene_on_top = (Engine::SceneManager::get_current_scene_name() == "TipsBookState") || TipsBookState::is_open;
    if (OldManDialogueState::consume_open_tips_request()) {
      if (!tips_scene_on_top && now_unscaled >= oldman_tips_request_cooldown_until_) {
        oldman_tips_request_cooldown_until_ = now_unscaled + 0.30f;
        TipsBookState::tips_button_id = tips_button_id;
        Log::info("[OldManFlow] Pushing TipsBookState from old-man request");
        Engine::SceneManager::push_scene("TipsBookState");
        return;
      }
      Log::info("[OldManFlow] Ignored old-man tips request (tips_scene_on_top={}, cooldown_left={})",
        tips_scene_on_top ? 1 : 0,
        (oldman_tips_request_cooldown_until_ > now_unscaled) ? (oldman_tips_request_cooldown_until_ - now_unscaled) : 0.0f);
    }

    // UI elements use screen-space coordinates (no camera tracking needed)
    // Get mouse position in screen/viewport coordinates for UI click detection
    glm::vec2 mouse_pos = Engine::Input::get_mouse_position();

    // Pause button click detection (screen coords)
    if (pause_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(pause_button_id))) {
      auto* pause_transform = Engine::ECS::Entity(pause_button_id).get_component<Components::Transform2D>();
      auto* pause_ui        = Engine::ECS::Entity(pause_button_id).get_component<Components::UI>();
      if (pause_transform && pause_ui) {
        auto screen_pos = Engine::Systems::compute_ui_position(*pause_ui, *pause_transform);
        float half_w = pause_transform->scale.x / 2.0f;
        float half_h = pause_transform->scale.y / 2.0f;

        if (mouse_pos.x >= screen_pos.x - half_w &&
          mouse_pos.x <= screen_pos.x + half_w &&
          mouse_pos.y >= screen_pos.y - half_h &&
          mouse_pos.y <= screen_pos.y + half_h) {
          if (Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
            Engine::SceneManager::push_scene("PauseState");
            return;
          }
        }
      }
    }

    // Tips button click detection (screen coords)
    if (tips_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) {
      auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
      auto* tips_ui        = Engine::ECS::Entity(tips_button_id).get_component<Components::UI>();
      if (tips_transform && tips_ui) {
        auto screen_pos = Engine::Systems::compute_ui_position(*tips_ui, *tips_transform);
        float half_w = tips_transform->scale.x / 2.0f;
        float half_h = tips_transform->scale.y / 2.0f;

        if (mouse_pos.x >= screen_pos.x - half_w &&
          mouse_pos.x <= screen_pos.x + half_w &&
          mouse_pos.y >= screen_pos.y - half_h &&
          mouse_pos.y <= screen_pos.y + half_h) {
          if (Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
            if (TipsBookState::is_open || Engine::SceneManager::get_current_scene_name() == "TipsBookState") return;
            TipsBookState::tips_button_id = tips_button_id;
            Engine::SceneManager::push_scene("TipsBookState");
            return;
          }
        }
      }
    }

    // Tips keyboard shortcut
    if (Engine::Input::is_pressed(Engine::KeyCode::I)) {
      if (TipsBookState::is_open || Engine::SceneManager::get_current_scene_name() == "TipsBookState") return;
      TipsBookState::tips_button_id = tips_button_id;
      Engine::SceneManager::push_scene("TipsBookState");
      return;
    }

    // Check if pause button is pressed (keyboard)
    if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
      Engine::SceneManager::push_scene("PauseState");
      return;
    }

    // Drive upgrade state machine (handles Tab key + upgrade button click)
    Engine::UpgradeUIStateMachine::instance().update();

  };

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

  void set_tips_button_hud_position() {
    set_tips_button_position(TIPS_BUTTON_HUD_X, TIPS_BUTTON_HUD_Y);
  }

  void set_tips_button_size(float size) {
    if (tips_button_id == 0 || !Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) return;
    auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
    if (!tips_transform) return;
    tips_transform->scale.x = size;
    tips_transform->scale.y = size;
  }

  void draw() {
    // Delegates to Engine EXE side — keeps all ImGui calls out of the DLL.
    Engine::TempUpgradeUIManager::draw();
  };

  void free() {
    Engine::UpgradeUIStateMachine::instance().set_picker_open_callback({});
    Engine::UpgradeUIStateMachine::instance().reset();
  };

  void unload() {
    Engine::AudioSystem::stopSceneMusic(true);
  };
};