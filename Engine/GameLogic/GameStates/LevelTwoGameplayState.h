//-----------------------------------------------------------------------------------
// file:    LevelTwoGameplayState.h
// project: Monkey Stole My GPA
//
// brief:   Dedicated gameplay scene for LevelTwo.
//          This is intentionally separate from GameplayState (LevelOne).
//-----------------------------------------------------------------------------------
#pragma once

/* EVERY CODE HERE IS TEMPORARY */

#include "pch.h"

#include "TipsBookState.h"
#include "UpgradePickerState.h"
#include "OldManDialogueState.h"
#include "GameplayState.h" // for shared UI/gameplay patterns and dependencies

// NOTE:
// GameplayState is header-only and contains substantial gameplay/UI logic.
// We intentionally keep a separate scene for LevelTwo so LevelOne and LevelTwo
// do not run under the same scene name/state.
//
// This class mirrors GameplayState but uses a different default level path.
class LevelTwoGameplayState : public Engine::Scene
{
  static constexpr const char* DEFAULT_LEVEL_PATH = "levels/LevelTwo.json";

  // Keep constants in sync with GameplayState
  static constexpr const char* GAMEPLAY_BGM_EVENT = "event:/BGM/LevelBGM";
  static constexpr const char* GAMEPLAY_BATTLECRY_EVENT = "event:/SFX/PlayerStartCry";
  static constexpr float TIPS_BUTTON_HUD_X = -227.0f;
  static constexpr float TIPS_BUTTON_HUD_Y = 92.0f;
  static constexpr float TIPS_BUTTON_HUD_SIZE = 220.0f;

  // UI entity IDs
  Engine::ECS::EntityID pause_button_id = 0;
  Engine::ECS::EntityID tips_button_id = 0;
  float oldman_tips_request_cooldown_until_ = 0.0f;
  Engine::ECS::EntityID upgrade_btn_id_[4] = { 0, 0, 0, 0 };
  int current_upgrade_threshold_ = 2;
  int current_kills_for_upgrade_ = 0;
  float upgrade_btn_anim_time_ = 0.0f;

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
    // LevelTwoGameplayState is LevelTwo-only.
    if (Engine::LevelManager::get_current_level_path().empty()) {
      Engine::LevelManager::open_level(DEFAULT_LEVEL_PATH);
    }

    current_upgrade_threshold_ = 2;
    current_kills_for_upgrade_ = 0;
    upgrade_btn_anim_time_ = 0.0f;

    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    // TODO: REMOVE
    player_entity.add_component<Components::Health>(5, 5);

    // Victory zone - triggers victory screen when player enters
    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(14600.0f, 830.0f, 0.0f, 100.0f, 200.0f)
      .add_component<Components::Interactable>(50.0f, "show_victory", "", "")
      .add_component<Components::RuntimeOnly>()
      .add_to_layer("effect");

    // Pause button
    auto pause_btn = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(-100.0f, 90.0f, 0.0f, 100.0f, 100.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Pause.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>(1.0f, 0.0f)
      .add_component<Components::RuntimeOnly>()
      .add_to_layer("main_menu_4");
    pause_button_id = pause_btn.get_id();

    // Tips book button (reuse if present)
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

    // Upgrade buttons
    auto make_upgrade_btn = [&](const char* img_path, int frame_w, int num_frames) -> Engine::ECS::EntityID {
      auto e = Engine::ECS::create_entity()
        .add_component<Components::Transform2D>(128.0f, -128.0f, 0.0f, 64.0f * 4.0f, 64.0f * 4.0f)
        .add_component<Components::Texture>(Engine::Resources::load_texture(img_path))
        .add_component<Components::TextureAtlas>(frame_w, frame_w, 1, num_frames, num_frames)
        .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
        .add_component<Components::UI>(0.0f, 1.0f)
        .add_component<Components::RuntimeOnly>()
        .add_to_layer("main_menu_4");
      return e.get_id();
      };

    upgrade_btn_id_[0] = make_upgrade_btn("images/Upgrades/skills_upgrade_2.png", 64, 11);
    upgrade_btn_id_[1] = make_upgrade_btn("images/Upgrades/skills_upgrade_3.png", 64, 12);
    upgrade_btn_id_[2] = make_upgrade_btn("images/Upgrades/skills_upgrade_4.png", 64, 13);
    upgrade_btn_id_[3] = make_upgrade_btn("images/Upgrades/skills_upgrade_5.png", 64, 14);

    Engine::UpgradeUIStateMachine::instance().set_upgrade_button(upgrade_btn_id_[0]);
    Engine::UpgradeUIStateMachine::instance().set_picker_open_callback([]() {
      Engine::SceneManager::push_scene("UpgradePickerState");
      });
  };

  void init() {
    Engine::AudioSystem::playSceneMusic(GAMEPLAY_BGM_EVENT, 0.3f, true);
    Engine::AudioSystem::playEvent(GAMEPLAY_BATTLECRY_EVENT);
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

    glm::vec2 mouse_pos = Engine::Input::get_mouse_position();

    // Pause button
    if (pause_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(pause_button_id))) {
      auto* pause_transform = Engine::ECS::Entity(pause_button_id).get_component<Components::Transform2D>();
      auto* pause_ui = Engine::ECS::Entity(pause_button_id).get_component<Components::UI>();
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

    // Tips button
    if (tips_button_id != 0 && Engine::ECS::is_entity_active(Engine::ECS::Entity(tips_button_id))) {
      auto* tips_transform = Engine::ECS::Entity(tips_button_id).get_component<Components::Transform2D>();
      auto* tips_ui = Engine::ECS::Entity(tips_button_id).get_component<Components::UI>();
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

    if (Engine::Input::is_pressed(Engine::KeyCode::I)) {
      if (TipsBookState::is_open || Engine::SceneManager::get_current_scene_name() == "TipsBookState") return;
      TipsBookState::tips_button_id = tips_button_id;
      Engine::SceneManager::push_scene("TipsBookState");
      return;
    }

    if (Engine::Input::is_pressed(Engine::KeyCode::Escape)) {
      Engine::SceneManager::push_scene("PauseState");
      return;
    }

    Engine::UpgradeUIStateMachine::instance().update();
    update_upgrade_button();
  };

  void update_upgrade_button() {
    auto& mgr = Engine::UpgradeManager::instance();
    int threshold = mgr.get_next_threshold();
    int kills = mgr.get_points();

    int threshold_idx = threshold - 2;
    if (threshold_idx < 0) threshold_idx = 0;
    if (threshold_idx > 3) threshold_idx = 3;

    int num_frames_per_sheet[4] = { 11, 12, 13, 14 };
    int num_fill_frames = threshold;
    int num_anim_frames = num_frames_per_sheet[threshold_idx] - num_fill_frames;

    int display_frame;
    if (kills >= threshold) {
      float anim_fps = 10.0f;
      upgrade_btn_anim_time_ += Engine::Time::get_unscaled_fixed_dt();
      int anim_frame = static_cast<int>(upgrade_btn_anim_time_ * anim_fps) % (num_anim_frames - 1);
      display_frame = num_fill_frames + 1 + anim_frame;
    }
    else {
      upgrade_btn_anim_time_ = 0.0f;
      display_frame = kills;
    }

    if (threshold_idx != current_upgrade_threshold_ || display_frame != current_kills_for_upgrade_) {
      current_upgrade_threshold_ = threshold_idx;
      current_kills_for_upgrade_ = display_frame;

      for (int i = 0; i < 4; ++i) {
        if (upgrade_btn_id_[i] == 0) continue;
        auto entity = Engine::ECS::Entity(upgrade_btn_id_[i]);
        auto* col = entity.get_component<Components::Color>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();
        if (i == threshold_idx) {
          if (col) col->a = 1.0f;
          if (atlas) atlas->drawIndex = display_frame;
        }
        else {
          if (col) col->a = 0.0f;
        }
      }

      Engine::UpgradeUIStateMachine::instance().set_upgrade_button(upgrade_btn_id_[threshold_idx]);
    }
    else if (kills >= threshold) {
      auto entity = Engine::ECS::Entity(upgrade_btn_id_[threshold_idx]);
      auto* atlas = entity.get_component<Components::TextureAtlas>();
      if (atlas) atlas->drawIndex = display_frame;
      current_kills_for_upgrade_ = display_frame;
    }
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
    Engine::TempUpgradeUIManager::draw();
  };

  void free() {
    Engine::UpgradeUIStateMachine::instance().set_picker_open_callback({});
    Engine::UpgradeUIStateMachine::instance().reset();
    upgrade_btn_anim_time_ = 0.0f;
  };

  void unload() {
    Engine::AudioSystem::stopSceneMusic(true);
  };
};
