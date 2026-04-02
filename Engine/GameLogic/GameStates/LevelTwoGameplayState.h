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
  static constexpr const char* PAUSE_BUTTON_NAME = "PauseButtonHUD";
  static constexpr float TIPS_BUTTON_HUD_X = -227.0f;
  static constexpr float TIPS_BUTTON_HUD_Y = 92.0f;
  static constexpr float TIPS_BUTTON_HUD_SIZE = 220.0f;
  static constexpr float PAUSE_BUTTON_HUD_X = -100.0f;
  static constexpr float PAUSE_BUTTON_HUD_Y = 90.0f;
  static constexpr float PAUSE_BUTTON_HUD_SIZE = 100.0f;
  static constexpr float UPGRADE_BUTTON_HUD_X = 128.0f;
  static constexpr float UPGRADE_BUTTON_HUD_Y = -128.0f;
  static constexpr float UPGRADE_BUTTON_HUD_SIZE = 256.0f;

  // UI entity IDs
  Engine::ECS::EntityID pause_button_id = 0;
  Engine::ECS::EntityID tips_button_id = 0;

  // Ambient train bump shake
  float ambient_shake_next_time_ = 0.0f;   // 3-second hard bump
  float train_rumble_next_time_  = 0.0f;   // 0.5-second low rumble
  float oldman_tips_request_cooldown_until_ = 0.0f;
  Engine::ECS::EntityID upgrade_btn_id_[4] = { 0, 0, 0, 0 };
  int current_upgrade_threshold_ = 2;
  int current_kills_for_upgrade_ = 0;
  float upgrade_btn_anim_time_ = 0.0f;

  bool is_matching_active_ui_entity(Engine::ECS::EntityID id, const char* texture_path) const {
    if (id == 0) {
      return false;
    }

    Engine::ECS::Entity entity(id);
    if (!entity.is_valid() || !Engine::ECS::is_entity_active(entity) || !entity.has_component<Components::UI>()) {
      return false;
    }

    auto* tex = entity.get_component<Components::Texture>();
    return tex && tex->texture_handle && Engine::Resources::get_relative_path(tex->texture_handle->path) == texture_path;
  }

  const char* get_upgrade_button_name(int index) const {
    static const char* names[4] = {
      "UpgradeButtonHUD2",
      "UpgradeButtonHUD3",
      "UpgradeButtonHUD4",
      "UpgradeButtonHUD5",
    };
    return names[index];
  }

  const char* get_upgrade_button_path(int index) const {
    static const char* paths[4] = {
      "images/Upgrades/skills_upgrade_2.png",
      "images/Upgrades/skills_upgrade_3.png",
      "images/Upgrades/skills_upgrade_4.png",
      "images/Upgrades/skills_upgrade_5.png",
    };
    return paths[index];
  }

  int get_upgrade_button_frame_count(int index) const {
    static const int frame_counts[4] = { 11, 12, 13, 14 };
    return frame_counts[index];
  }

  Engine::ECS::Entity find_ui_entity_by_name_or_texture(const char* entity_name, const char* texture_path) {
    Engine::ECS::Entity found;
    Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
      if (found.is_valid()) return;
      if (!Engine::ECS::is_entity_active(entity)) return;
      if (entity.get_name() == entity_name) {
        found = entity;
      }
      });

    if (found.is_valid()) return found;

    Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
      if (found.is_valid()) return;
      if (!Engine::ECS::is_entity_active(entity)) return;
      if (!entity.has_component<Components::UI>()) return;
      auto* tex = entity.get_component<Components::Texture>();
      if (!tex || !tex->texture_handle) return;
      if (Engine::Resources::get_relative_path(tex->texture_handle->path) == texture_path) {
        found = entity;
      }
      });

    return found;
  }

  Engine::ECS::Entity find_pause_button_entity() {
    return find_ui_entity_by_name_or_texture(PAUSE_BUTTON_NAME, "images/UI/Pause.png");
  }

  Engine::ECS::Entity find_tips_button_entity() {
    return find_ui_entity_by_name_or_texture("TipsButtonHUD", "images/UI/TipsBook.png");
  }

  Engine::ECS::Entity find_upgrade_button_entity(int index) {
    return find_ui_entity_by_name_or_texture(get_upgrade_button_name(index), get_upgrade_button_path(index));
  }

  void restore_pause_button_hud(Engine::ECS::Entity pause_btn) {
    if (!pause_btn.is_valid()) return;

    pause_btn.set_name(PAUSE_BUTTON_NAME);
    pause_btn.clear_layers().add_to_layer("gameplay_ui_2");

    if (auto* tr = pause_btn.get_component<Components::Transform2D>()) {
      tr->position.x = PAUSE_BUTTON_HUD_X;
      tr->position.y = PAUSE_BUTTON_HUD_Y;
      tr->scale.x = PAUSE_BUTTON_HUD_SIZE;
      tr->scale.y = PAUSE_BUTTON_HUD_SIZE;
    }
    if (auto* ui = pause_btn.get_component<Components::UI>()) {
      ui->anchor = { 1.0f, 0.0f };
    }
    if (auto* tex = pause_btn.get_component<Components::Texture>()) {
      *tex = Components::Texture(Engine::Resources::load_texture("images/UI/Pause.png"));
    }
    if (auto* col = pause_btn.get_component<Components::Color>()) {
      *col = Components::Color(1.0f, 1.0f, 1.0f, 1.0f);
    }
  }

  Engine::ECS::EntityID create_pause_button_entity() {
    auto pause_btn = Engine::ECS::create_entity()
      .set_name(PAUSE_BUTTON_NAME)
      .add_component<Components::Transform2D>(PAUSE_BUTTON_HUD_X, PAUSE_BUTTON_HUD_Y, 0.0f, PAUSE_BUTTON_HUD_SIZE, PAUSE_BUTTON_HUD_SIZE)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/Pause.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>(1.0f, 0.0f)
      .add_component<Components::RuntimeOnly>()
      .add_to_layer("gameplay_ui_2");
    return pause_btn.get_id();
  }

  void restore_tips_button_hud(Engine::ECS::Entity tips_btn) {
    if (!tips_btn.is_valid()) return;

    tips_btn.set_name("TipsButtonHUD");
    tips_btn.clear_layers().add_to_layer("gameplay_ui_2");

    if (auto* tr = tips_btn.get_component<Components::Transform2D>()) {
      tr->position.x = TIPS_BUTTON_HUD_X;
      tr->position.y = TIPS_BUTTON_HUD_Y;
      tr->scale.x = TIPS_BUTTON_HUD_SIZE;
      tr->scale.y = TIPS_BUTTON_HUD_SIZE;
    }
    if (auto* ui = tips_btn.get_component<Components::UI>()) {
      ui->anchor = { 1.0f, 0.0f };
    }
    if (auto* tex = tips_btn.get_component<Components::Texture>()) {
      *tex = Components::Texture(Engine::Resources::load_texture("images/UI/TipsBook.png"));
    }
    if (auto* col = tips_btn.get_component<Components::Color>()) {
      *col = Components::Color(1.0f, 1.0f, 1.0f, 1.0f);
    }
  }

  Engine::ECS::EntityID create_tips_button_entity() {
    auto tips_btn = Engine::ECS::create_entity()
      .set_name("TipsButtonHUD")
      .add_component<Components::Transform2D>(TIPS_BUTTON_HUD_X, TIPS_BUTTON_HUD_Y, 0.0f, TIPS_BUTTON_HUD_SIZE, TIPS_BUTTON_HUD_SIZE)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/UI/TipsBook.png"))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>(1.0f, 0.0f)
      .add_component<Components::RuntimeOnly>()
      .add_to_layer("gameplay_ui_2");
    return tips_btn.get_id();
  }

  void restore_upgrade_button_hud(Engine::ECS::Entity upgrade_btn, int index) {
    if (!upgrade_btn.is_valid()) return;

    upgrade_btn.set_name(get_upgrade_button_name(index));
    upgrade_btn.clear_layers().add_to_layer("gameplay_ui_2");

    if (auto* tr = upgrade_btn.get_component<Components::Transform2D>()) {
      tr->position.x = UPGRADE_BUTTON_HUD_X;
      tr->position.y = UPGRADE_BUTTON_HUD_Y;
      tr->scale.x = UPGRADE_BUTTON_HUD_SIZE;
      tr->scale.y = UPGRADE_BUTTON_HUD_SIZE;
    }
    if (auto* ui = upgrade_btn.get_component<Components::UI>()) {
      ui->anchor = { 0.0f, 1.0f };
    }
    if (auto* tex = upgrade_btn.get_component<Components::Texture>()) {
      *tex = Components::Texture(Engine::Resources::load_texture(get_upgrade_button_path(index)));
    }
  }

  Engine::ECS::EntityID create_upgrade_button_entity(int index) {
    auto upgrade_btn = Engine::ECS::create_entity()
      .set_name(get_upgrade_button_name(index))
      .add_component<Components::Transform2D>(UPGRADE_BUTTON_HUD_X, UPGRADE_BUTTON_HUD_Y, 0.0f, UPGRADE_BUTTON_HUD_SIZE, UPGRADE_BUTTON_HUD_SIZE)
      .add_component<Components::Texture>(Engine::Resources::load_texture(get_upgrade_button_path(index)))
      .add_component<Components::TextureAtlas>(64, 64, 1, get_upgrade_button_frame_count(index), get_upgrade_button_frame_count(index))
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 0.0f)
      .add_component<Components::UI>(0.0f, 1.0f)
      .add_component<Components::RuntimeOnly>()
      .add_to_layer("gameplay_ui_2");
    return upgrade_btn.get_id();
  }

  void bind_upgrade_overlay_state() {
    int threshold_idx = Engine::UpgradeManager::instance().get_next_threshold() - 2;
    if (threshold_idx < 0) threshold_idx = 0;
    if (threshold_idx > 3) threshold_idx = 3;

    if (upgrade_btn_id_[threshold_idx] != 0) {
      Engine::UpgradeUIStateMachine::instance().set_upgrade_button(upgrade_btn_id_[threshold_idx]);
    }

    Engine::UpgradeUIStateMachine::instance().set_picker_open_callback([]() {
      Engine::SceneManager::push_scene("UpgradePickerState");
      });
  }

  void ensure_overlay_entities() {
    bool snapshot_needs_rebuild = false;
    bool upgrade_binding_changed = false;

    if (!is_matching_active_ui_entity(pause_button_id, "images/UI/Pause.png")) {
      auto pause_btn = find_pause_button_entity();
      if (pause_btn.is_valid()) {
        if (pause_btn.get_name() != PAUSE_BUTTON_NAME) {
          pause_btn.set_name(PAUSE_BUTTON_NAME);
          snapshot_needs_rebuild = true;
        }
        restore_pause_button_hud(pause_btn);
        pause_button_id = pause_btn.get_id();
      }
      else {
        pause_button_id = create_pause_button_entity();
        snapshot_needs_rebuild = true;
      }
    }

    if (!is_matching_active_ui_entity(tips_button_id, "images/UI/TipsBook.png")) {
      auto tips_btn = find_tips_button_entity();
      if (tips_btn.is_valid()) {
        if (tips_btn.get_name() != "TipsButtonHUD") {
          tips_btn.set_name("TipsButtonHUD");
          snapshot_needs_rebuild = true;
        }
        restore_tips_button_hud(tips_btn);
        tips_button_id = tips_btn.get_id();
      }
      else {
        tips_button_id = create_tips_button_entity();
        snapshot_needs_rebuild = true;
      }
    }

    for (int i = 0; i < 4; ++i) {
      if (is_matching_active_ui_entity(upgrade_btn_id_[i], get_upgrade_button_path(i))) {
        continue;
      }

      auto upgrade_btn = find_upgrade_button_entity(i);
      if (upgrade_btn.is_valid()) {
        if (upgrade_btn.get_name() != get_upgrade_button_name(i)) {
          upgrade_btn.set_name(get_upgrade_button_name(i));
          snapshot_needs_rebuild = true;
        }
        restore_upgrade_button_hud(upgrade_btn, i);
        if (upgrade_btn_id_[i] != upgrade_btn.get_id()) {
          upgrade_binding_changed = true;
        }
        upgrade_btn_id_[i] = upgrade_btn.get_id();
      }
      else {
        upgrade_btn_id_[i] = create_upgrade_button_entity(i);
        snapshot_needs_rebuild = true;
        upgrade_binding_changed = true;
      }
    }

    if (upgrade_binding_changed) {
      current_upgrade_threshold_ = -1;
      current_kills_for_upgrade_ = -1;
      upgrade_btn_anim_time_ = 0.0f;
    }

    bind_upgrade_overlay_state();

    if (snapshot_needs_rebuild) {
      Engine::LevelManager::rebuild_active_snapshot();
    }
  }

public:
  void load() {
    // LevelTwoGameplayState is LevelTwo-only.
    if (Engine::LevelManager::get_current_level_path().empty()) {
      Engine::Resources::open_level(DEFAULT_LEVEL_PATH);
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
    pause_button_id = create_pause_button_entity();

    // Tips book button (reuse if present)
    auto tips_btn = find_tips_button_entity();
    if (tips_btn.is_valid()) {
      tips_button_id = tips_btn.get_id();
      restore_tips_button_hud(tips_btn);
    }
    else {
      tips_button_id = create_tips_button_entity();
    }

    // Upgrade buttons
    for (int i = 0; i < 4; ++i) {
      upgrade_btn_id_[i] = create_upgrade_button_entity(i);
    }

    bind_upgrade_overlay_state();

    Engine::LevelManager::rebuild_active_snapshot();
  };

  void init() {
    Engine::AudioSystem::playSceneMusic(GAMEPLAY_BGM_EVENT, 0.3f, true);
    Engine::AudioSystem::playEvent(GAMEPLAY_BATTLECRY_EVENT);

    // Schedule first ambient train shakes
    ambient_shake_next_time_ = Engine::Time::get_time() + 3.0f;
    train_rumble_next_time_  = Engine::Time::get_time() + 0.5f;
  };

  void update() {
    ensure_overlay_entities();

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

    const bool pause_shortcut_pressed = Engine::Input::is_pressed_raw(Engine::KeyCode::Escape) ||
      Engine::Input::is_pressed(Engine::KeyCode::Escape);
    if (pause_shortcut_pressed) {
      Engine::SceneManager::push_scene("PauseState");
      return;
    }

    Engine::UpgradeUIStateMachine::instance().update();
    update_upgrade_button();

    // Train shakes - intervals are long enough that combat shakes (decay ~0.4s)
    // always clear before the next train shake fires, so they never stack.
    {
      const float now = Engine::Time::get_time();

      // Continuous low-level rumble every 0.5s (combat shakes decay in ~0.4s, so no overlap)
      if (now >= train_rumble_next_time_) {
        Engine::Systems::add_camera_shake(0.01f);
        train_rumble_next_time_ = now + 0.5f;
      }

      // Periodic hard bump every 3 seconds
      if (now >= ambient_shake_next_time_) {
        Engine::Systems::add_camera_shake(0.2f);
        ambient_shake_next_time_ = now + 3.0f;
      }
    }
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
    ambient_shake_next_time_ = 0.0f;
    train_rumble_next_time_  = 0.0f;
  };

  void unload() {
    Engine::AudioSystem::stopSceneMusic(true);
  };
};
