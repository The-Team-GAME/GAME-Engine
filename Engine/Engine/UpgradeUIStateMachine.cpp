//-----------------------------------------------------------------------------------
// file:    UpgradeUIStateMachine.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the UpgradeUIStateMachine. Drives the upgrade flow
//          states (Idle/Available/Rolling/PickerOpen/Applying/Skipping) and
//          spawns/destroys entity-based UI elements for the card picker.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "UpgradeUIStateMachine.h"
#include "UpgradeManager.h"
#include "ECS/ECS.h"
#include "Components/Components.h"
#include "Resources.h"
#include "InputManager.h"
#include "Time.h"
#include "Logger.h"
#include "FileIO/Settings.h"
#include "FileIO/SettingsManager.h"
#include "Easing.h"
#include <cmath>

namespace Engine
{
  // ---- Card animation toggles are now runtime members on the singleton ----
  // (enable_flip_reveal / enable_idle_float / enable_hover_lift / enable_selection_stamp)

  // ---- Singleton ----

  namespace { UpgradeUIStateMachine* g_upgrade_ui_sm = nullptr; }

  UpgradeUIStateMachine& UpgradeUIStateMachine::instance()
  {
    if (!g_upgrade_ui_sm) g_upgrade_ui_sm = new UpgradeUIStateMachine();
    return *g_upgrade_ui_sm;
  }

  void UpgradeUIStateMachine::destroy_instance()
  {
    delete g_upgrade_ui_sm;
    g_upgrade_ui_sm = nullptr;
  }

  // ---- State queries ----

  UpgradeFlowState UpgradeUIStateMachine::get_state() const
  {
    return state_;
  }

  void UpgradeUIStateMachine::set_upgrade_button(ECS::EntityID id)
  {
    upgrade_btn_id_ = id;
  }

  // ---- Transition logic ----

  void UpgradeUIStateMachine::transition_to(UpgradeFlowState new_state)
  {
    if (new_state == state_) return;
    on_exit(state_);
    UpgradeFlowState old = state_;
    state_ = new_state;
    on_enter(new_state);
    Log::info("[UpgradeUI] {} -> {}",
      static_cast<int>(old), static_cast<int>(new_state));
  }

  void UpgradeUIStateMachine::on_exit(UpgradeFlowState old_state)
  {
    switch (old_state)
    {
    case UpgradeFlowState::Available:
    {
      // Hide upgrade button when leaving Available
      if (upgrade_btn_id_ != 0 && ECS::is_entity_active(ECS::Entity(upgrade_btn_id_)))
      {
        auto* c = ECS::Entity(upgrade_btn_id_).get_component<Components::Color>();
        if (c) c->a = 0.0f;
      }
      break;
    }
    case UpgradeFlowState::PickerOpen:
    {
      // Destroy picker entities and resume time
      destroy_picker_entities();
      Time::set_time_scale(1.0f);
      break;
    }
    default:
      break;
    }
  }

  void UpgradeUIStateMachine::on_enter(UpgradeFlowState new_state)
  {
    auto& mgr = UpgradeManager::instance();

    switch (new_state)
    {
    case UpgradeFlowState::Available:
    {
      // Show upgrade button
      if (upgrade_btn_id_ != 0 && ECS::is_entity_active(ECS::Entity(upgrade_btn_id_)))
      {
        auto* c = ECS::Entity(upgrade_btn_id_).get_component<Components::Color>();
        if (c) c->a = 1.0f;
      }
      break;
    }
    case UpgradeFlowState::Rolling:
    {
      // Roll cards immediately
      mgr.roll_cards();
      break;
    }
    case UpgradeFlowState::PickerOpen:
    {
      // Pause time and spawn picker UI
      Time::set_time_scale(0.0001f);
      spawn_picker_entities();
      break;
    }
    case UpgradeFlowState::Applying:
    {
      // Apply the picked upgrade
      mgr.pick_card(picked_slot_);
      picked_slot_ = -1;
      break;
    }
    case UpgradeFlowState::Skipping:
    {
      // Skip cards
      mgr.skip_cards();
      break;
    }
    default:
      break;
    }
  }

  // ---- Per-frame update ----

  void UpgradeUIStateMachine::update()
  {
    auto& mgr = UpgradeManager::instance();

    switch (state_)
    {
    case UpgradeFlowState::Idle:
    {
      if (mgr.can_upgrade())
        transition_to(UpgradeFlowState::Available);
      break;
    }
    case UpgradeFlowState::Available:
    {
      if (!mgr.can_upgrade())
      {
        transition_to(UpgradeFlowState::Idle);
        break;
      }

      // Check for 'I' key or mouse click on upgrade button
      bool open_picker = Input::is_pressed(KeyCode::I);

      if (!open_picker && upgrade_btn_id_ != 0
        && ECS::is_entity_active(ECS::Entity(upgrade_btn_id_)))
      {
        auto* t = ECS::Entity(upgrade_btn_id_).get_component<Components::Transform2D>();
        if (t)
        {
          glm::vec2 mouse = Input::get_mouse_position();
          float hw = t->scale.x / 2.0f;
          float hh = t->scale.y / 2.0f;
          if (mouse.x >= t->position.x - hw && mouse.x <= t->position.x + hw &&
            mouse.y >= t->position.y - hh && mouse.y <= t->position.y + hh)
          {
            if (Input::is_pressed(KeyCode::MouseLeft))
              open_picker = true;
          }
        }
      }

      if (open_picker)
        transition_to(UpgradeFlowState::Rolling);
      break;
    }
    case UpgradeFlowState::Rolling:
    {
      if (mgr.has_rolled_cards())
        transition_to(UpgradeFlowState::PickerOpen);
      else
        transition_to(UpgradeFlowState::Idle);
      break;
    }
    case UpgradeFlowState::PickerOpen:
    {
      // Drive card animations (flip, float, hover, stamp)
      animate_cards();

      // Don't process input during flip reveal or stamp animation
      if (!flip_done_) break;
      if (enable_selection_stamp && stamp_time_ >= 0.0f) break;

      glm::vec2 mouse = Input::get_mouse_position();

      // Hit-test card 0
      if (card0_id_ != 0 && ECS::is_entity_active(ECS::Entity(card0_id_)))
      {
        float hw = card_base_sx_[0] / 2.0f;
        float hh = card_base_sy_[0] / 2.0f;
        if (mouse.x >= card_base_x_[0] - hw && mouse.x <= card_base_x_[0] + hw &&
          mouse.y >= card_base_y_[0] - hh && mouse.y <= card_base_y_[0] + hh)
        {
          if (Input::is_pressed(KeyCode::MouseLeft))
          {
            if (enable_selection_stamp)
            {
              stamp_card_ = 0; stamp_time_ = 0.0f;
            }
            else
            {
              picked_slot_ = 0; transition_to(UpgradeFlowState::Applying);
            }
            break;
          }
        }
      }

      // Hit-test card 1
      if (card1_id_ != 0 && ECS::is_entity_active(ECS::Entity(card1_id_)))
      {
        float hw = card_base_sx_[1] / 2.0f;
        float hh = card_base_sy_[1] / 2.0f;
        if (mouse.x >= card_base_x_[1] - hw && mouse.x <= card_base_x_[1] + hw &&
          mouse.y >= card_base_y_[1] - hh && mouse.y <= card_base_y_[1] + hh)
        {
          if (Input::is_pressed(KeyCode::MouseLeft))
          {
            if (enable_selection_stamp)
            {
              stamp_card_ = 1; stamp_time_ = 0.0f;
            }
            else
            {
              picked_slot_ = 1; transition_to(UpgradeFlowState::Applying);
            }
            break;
          }
        }
      }

      break;
    }
    case UpgradeFlowState::Applying:
    case UpgradeFlowState::Skipping:
    {
      // Re-evaluate after applying or skipping
      if (mgr.can_upgrade())
        transition_to(UpgradeFlowState::Available);
      else
        transition_to(UpgradeFlowState::Idle);
      break;
    }
    }
  }

  // ---- Card animations ----

  void UpgradeUIStateMachine::animate_cards()
  {
    float dt = Time::get_unscaled_fixed_dt();
    picker_open_time_ += dt;

    ECS::EntityID card_ids[2] = { card0_id_, card1_id_ };

    // Start from base values each frame; effects accumulate offsets
    float pos_y[2] = { card_base_y_[0], card_base_y_[1] };
    float scale_x[2] = { card_base_sx_[0], card_base_sx_[1] };
    float scale_y[2] = { card_base_sy_[0], card_base_sy_[1] };

    // ---- Flip Reveal ----
    if (enable_flip_reveal && !flip_done_)
    {
      constexpr float flip_duration = 0.3f;
      constexpr float flip_stagger = 0.08f;
      bool all_done = true;
      for (int i = 0; i < 2; ++i)
      {
        float card_t = picker_open_time_ - i * flip_stagger;
        if (card_t < 0.0f)
        {
          scale_x[i] = 0.0f;
          all_done = false;
        }
        else if (card_t < flip_duration)
        {
          scale_x[i] = Easing::EaseBackOut(card_t, 0.0f, card_base_sx_[i], flip_duration);
          all_done = false;
        }
        // else: scale_x stays at base (fully revealed)
      }
      if (all_done) flip_done_ = true;
    }

    // ---- Idle Float (only after flip completes) ----
    if (enable_idle_float && flip_done_)
    {
      constexpr float float_amplitude = 3.0f;
      constexpr float float_speed = 0.2f;
      constexpr float float_phase_offset = 0.5f;
      constexpr float two_pi = 6.2831853f;
      for (int i = 0; i < 2; ++i)
      {
        pos_y[i] += float_amplitude * sinf(picker_open_time_ * float_speed * two_pi + i * float_phase_offset);
      }
    }

    // ---- Hover Lift (only after flip completes) ----
    if (enable_hover_lift && flip_done_)
    {
      constexpr float hover_lift_y = -15.0f;
      constexpr float hover_scale_boost = 1.05f;
      constexpr float hover_speed = 8.0f;

      glm::vec2 mouse = Input::get_mouse_position();
      hovered_card_ = -1;
      for (int i = 0; i < 2; ++i)
      {
        float hw = card_base_sx_[i] / 2.0f;
        float hh = card_base_sy_[i] / 2.0f;
        if (mouse.x >= card_base_x_[i] - hw && mouse.x <= card_base_x_[i] + hw &&
          mouse.y >= card_base_y_[i] - hh && mouse.y <= card_base_y_[i] + hh)
        {
          hovered_card_ = i;
        }
      }

      for (int i = 0; i < 2; ++i)
      {
        float target = (hovered_card_ == i) ? 1.0f : 0.0f;
        hover_lerp_[i] += (target - hover_lerp_[i]) * hover_speed * dt;
        // Clamp to [0,1]
        if (hover_lerp_[i] < 0.0f) hover_lerp_[i] = 0.0f;
        if (hover_lerp_[i] > 1.0f) hover_lerp_[i] = 1.0f;

        pos_y[i] += hover_lift_y * hover_lerp_[i];
        float boost = 1.0f + (hover_scale_boost - 1.0f) * hover_lerp_[i];
        scale_x[i] *= boost;
        scale_y[i] *= boost;
      }
    }

    // ---- Selection Stamp ----
    if (enable_selection_stamp && stamp_time_ >= 0.0f)
    {
      constexpr float stamp_duration = 0.15f;
      constexpr float stamp_squish = 0.85f;
      stamp_time_ += dt;

      if (stamp_time_ >= stamp_duration)
      {
        // Stamp finished — apply the upgrade
        stamp_time_ = -1.0f;
        picked_slot_ = stamp_card_;
        stamp_card_ = -1;
        transition_to(UpgradeFlowState::Applying);
        return;
      }

      int c = stamp_card_;
      if (c >= 0 && c < 2)
      {
        float half = stamp_duration / 2.0f;
        float squish_factor;
        if (stamp_time_ < half)
        {
          // Squish down
          float t = stamp_time_;
          squish_factor = Easing::EaseQuadIn(t, 1.0f, stamp_squish - 1.0f, half);
        }
        else
        {
          // Bounce back
          float t = stamp_time_ - half;
          squish_factor = Easing::EaseQuadOut(t, stamp_squish, 1.0f - stamp_squish, half);
        }
        scale_x[c] *= squish_factor;
        scale_y[c] *= squish_factor;
      }
    }

    // ---- Write final values to entities ----
    for (int i = 0; i < 2; ++i)
    {
      if (card_ids[i] == 0) continue;
      auto* t = ECS::Entity(card_ids[i]).get_component<Components::Transform2D>();
      if (!t) continue;
      t->position.x = card_base_x_[i];
      t->position.y = pos_y[i];
      t->scale.x = scale_x[i];
      t->scale.y = scale_y[i];
    }
  }

  // ---- Picker entity management ----

  void UpgradeUIStateMachine::spawn_picker_entities()
  {
    auto& mgr = UpgradeManager::instance();
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();

    auto rect = Resources::get_rect_texture();

    // ---- Spritesheet (Cards_Spritesheet.png: 1024 x 224) ----
    constexpr int   ss_img_w = 1024;  // full image width  in pixels
    constexpr int   ss_img_h = 224;   // full image height in pixels
    constexpr int   ss_cols = 8;     // number of columns in the sheet
    constexpr int   ss_rows = 1;     // number of rows    in the sheet
    constexpr int   ss_total = 6;     // number of cards in use (last 2 ignored for now)
    constexpr int   ss_card_w = ss_img_w / ss_cols;  // 128 — one frame width
    constexpr int   ss_card_h = ss_img_h / ss_rows;  // 224 — one frame height
    constexpr float ss_scale = 2.0f;                // display scale multiplier

    // ---- Panel (UpgradeMenu.png: 192 x 256) ----
    constexpr float panel_img_w = 192.0f;
    constexpr float panel_img_h = 256.0f;
    constexpr float panel_scale = 2.0f;  // display scale multiplier
    // -----------------------------------------------------------------------

    auto spritesheet = Resources::load_texture("images/Upgrades/Cards_Spritesheet.png");
    auto panel_tex = Resources::load_texture("images/Upgrades/UpgradeMenu.png");

    // Dimmer — full-screen dark overlay
    auto dimmer = ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(rect)
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.6f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_4");
    dimmer_id_ = dimmer.get_id();

    // Layout sizing — derived from image sizes and scale factors above
    constexpr float card_display_w = static_cast<float>(ss_card_w) * ss_scale;
    constexpr float card_display_h = static_cast<float>(ss_card_h) * ss_scale;
    constexpr float panel_display_w = panel_img_w * panel_scale;
    constexpr float panel_display_h = panel_img_h * panel_scale;
    constexpr float gap = 10.0f;

    // Total group: [panel] gap [card0] gap [card1]
    float group_w = panel_display_w + gap + card_display_w + gap + card_display_w;
    float group_left = (1600.0f - group_w) / 2.0f;  // left edge of group

    float panel_cx = group_left + panel_display_w / 2.0f;
    float card0_cx = group_left + panel_display_w + gap + card_display_w / 2.0f;
    float card1_cx = group_left + panel_display_w + gap + card_display_w + gap + card_display_w / 2.0f;
    constexpr float center_y = 450.0f;

    // "Your Upgrades" panel — left
    auto panel = ECS::create_entity()
      .add_component<Components::Transform2D>(panel_cx, center_y, 0.0f, panel_display_w, panel_display_h)
      .add_component<Components::Texture>(panel_tex)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    panel_id_ = panel.get_id();

    // ---- Mini-cards showing acquired upgrades on the panel ----
    // Each mini-card is 1/6 of the full display card size
    constexpr float mini_w = card_display_w / 6.0f;  // ~53.3
    constexpr float mini_h = card_display_h / 6.0f;  // ~74.7
    constexpr float mini_gap = 2.0f;

    // Starting positions for each category row (offsets from panel top-left corner).
    // Adjust these values to align with the category markers on the panel image.
    constexpr float row_start_x = 150.0f;         // X offset from panel left edge
    constexpr float row_start_y = 142.0f;         // Top row position
    constexpr float row_height = 94.0f;           // Consistent spacing between rows
    constexpr float row0_y = row_start_y;         // Basic Attack Y offset from panel top edge
    constexpr float row1_y = row0_y + row_height; // Uppercut Y offset from panel top edge
    constexpr float row2_y = row1_y + row_height; // Downsmash Y offset from panel top edge
    constexpr float row3_y = row2_y + row_height; // Final Row Y offset from panel top edge
    constexpr float row_y[4] = { row0_y, row1_y, row2_y, row3_y };

    float panel_left = panel_cx - panel_display_w / 2.0f;
    float panel_top = center_y - panel_display_h / 2.0f;

    // Map stat_name -> category row
    auto get_category = [](const std::string& stat_name) -> int {
      if (stat_name == "attack_damage")                                   return 0;
      if (stat_name == "uppercut_damage" || stat_name == "uppercut_cooldown")  return 1;
      if (stat_name == "downsmash_damage" || stat_name == "downsmash_cooldown") return 2;
      if (stat_name == "dash_cooldown")                                   return 3;
      return -1;
      };

    int row_card_count[4] = { 0, 0, 0, 0 };

    for (size_t i = 0; i < settings.upgrades.size(); ++i)
    {
      const auto& upg = settings.upgrades[i];
      int level = mgr.get_upgrade_level(upg.id);
      if (level <= 0) continue;

      int cat = get_category(upg.stat_name);
      if (cat < 0) continue;

      for (int lv = 0; lv < level; ++lv)
      {
        float x = panel_left + row_start_x + mini_w / 2.0f
          + row_card_count[cat] * (mini_w + mini_gap);
        float y = panel_top + row_y[cat];

        auto mc = ECS::create_entity()
          .add_component<Components::Transform2D>(x, y, 0.0f, mini_w, mini_h)
          .add_component<Components::Texture>(spritesheet)
          .add_component<Components::TextureAtlas>(ss_card_w, ss_card_h, ss_rows, ss_cols, ss_total)
          .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
          .add_component<Components::UI>()
          .add_to_layer("main_menu_5");
        if (auto* atlas = ECS::Entity(mc.get_id()).get_component<Components::TextureAtlas>())
          atlas->drawIndex = upg.card_index;
        mini_card_ids_.push_back(mc.get_id());

        ++row_card_count[cat];
      }
    }

    // Card 0 — center
    int idx0 = mgr.get_rolled_card(0);
    int draw0 = (idx0 >= 0 && idx0 < static_cast<int>(settings.upgrades.size()))
      ? settings.upgrades[idx0].card_index : 0;

    auto c0 = ECS::create_entity()
      .add_component<Components::Transform2D>(card0_cx, center_y, 0.0f, card_display_w, card_display_h)
      .add_component<Components::Texture>(spritesheet)
      .add_component<Components::TextureAtlas>(ss_card_w, ss_card_h, ss_rows, ss_cols, ss_total)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    if (auto* atlas = ECS::Entity(c0.get_id()).get_component<Components::TextureAtlas>())
      atlas->drawIndex = draw0;
    card0_id_ = c0.get_id();

    // Card 1 — right
    int idx1 = mgr.get_rolled_card(1);
    int draw1 = (idx1 >= 0 && idx1 < static_cast<int>(settings.upgrades.size()))
      ? settings.upgrades[idx1].card_index : 0;

    auto c1 = ECS::create_entity()
      .add_component<Components::Transform2D>(card1_cx, center_y, 0.0f, card_display_w, card_display_h)
      .add_component<Components::Texture>(spritesheet)
      .add_component<Components::TextureAtlas>(ss_card_w, ss_card_h, ss_rows, ss_cols, ss_total)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("main_menu_5");
    if (auto* atlas = ECS::Entity(c1.get_id()).get_component<Components::TextureAtlas>())
      atlas->drawIndex = draw1;
    card1_id_ = c1.get_id();

    // ---- Store base values and reset animation state ----
    card_base_x_[0] = card0_cx;    card_base_y_[0] = center_y;
    card_base_sx_[0] = card_display_w; card_base_sy_[0] = card_display_h;
    card_base_x_[1] = card1_cx;    card_base_y_[1] = center_y;
    card_base_sx_[1] = card_display_w; card_base_sy_[1] = card_display_h;

    picker_open_time_ = 0.0f;
    hovered_card_ = -1;
    hover_lerp_[0] = 0.0f;
    hover_lerp_[1] = 0.0f;
    stamp_time_ = -1.0f;
    stamp_card_ = -1;

    if (enable_flip_reveal)
    {
      flip_done_ = false;
      // Start cards invisible (scale.x = 0) for flip-in
      if (auto* t0 = ECS::Entity(card0_id_).get_component<Components::Transform2D>())
        t0->scale.x = 0.0f;
      if (auto* t1 = ECS::Entity(card1_id_).get_component<Components::Transform2D>())
        t1->scale.x = 0.0f;
    }
    else
    {
      flip_done_ = true;
    }
  }

  void UpgradeUIStateMachine::destroy_picker_entities()
  {
    auto destroy_if_active = [](ECS::EntityID& id)
      {
        if (id != 0 && ECS::is_entity_active(ECS::Entity(id)))
          ECS::destroy_entity(ECS::Entity(id));
        id = 0;
      };

    destroy_if_active(dimmer_id_);
    destroy_if_active(panel_id_);
    for (auto& id : mini_card_ids_)
      destroy_if_active(id);
    mini_card_ids_.clear();
    destroy_if_active(card0_id_);
    destroy_if_active(card1_id_);
  }

  // ---- Reset ----

  void UpgradeUIStateMachine::reset()
  {
    if (state_ == UpgradeFlowState::PickerOpen)
    {
      destroy_picker_entities();
      Time::set_time_scale(1.0f);
    }
    state_ = UpgradeFlowState::Idle;
    picked_slot_ = -1;
  }

} // namespace Engine
