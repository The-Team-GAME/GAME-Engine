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
#include "Audio/AudioSystem.h"
#include "FileIO/Settings.h"
#include "FileIO/SettingsManager.h"
#include "Easing.h"
#include "Graphics/Renderer.h"
#include <cmath>
#include <random>

namespace Engine
{
  // ---- Card animation toggles are now runtime members on the singleton ----
  // (enable_flip_reveal / enable_idle_float / enable_hover_lift / enable_selection_stamp)

  // ---- Singleton ----

  namespace {
    UpgradeUIStateMachine* g_upgrade_ui_sm = nullptr;
    static constexpr const char* UPGRADE_LOWPASS_SNAPSHOT_INSTANCE = "upgrade_lowpass_snapshot";
    static constexpr const char* UPGRADE_LOWPASS_SNAPSHOT_EVENT = "snapshot:/LowpassSnapshot";
    static bool gUpgradeLowpassSnapshotActive = false;

    void start_upgrade_lowpass_snapshot()
    {
      if (gUpgradeLowpassSnapshotActive) return;

      if (!Engine::AudioSystem::createEventInstance(UPGRADE_LOWPASS_SNAPSHOT_INSTANCE, UPGRADE_LOWPASS_SNAPSHOT_EVENT)) {
        return;
      }

      if (!Engine::AudioSystem::startEventInstance(UPGRADE_LOWPASS_SNAPSHOT_INSTANCE)) {
        Engine::AudioSystem::releaseEventInstance(UPGRADE_LOWPASS_SNAPSHOT_INSTANCE);
        return;
      }

      gUpgradeLowpassSnapshotActive = true;
    }

    void stop_upgrade_lowpass_snapshot()
    {
      if (!gUpgradeLowpassSnapshotActive) return;

      Engine::AudioSystem::stopEventInstance(UPGRADE_LOWPASS_SNAPSHOT_INSTANCE, false);
      Engine::AudioSystem::releaseEventInstance(UPGRADE_LOWPASS_SNAPSHOT_INSTANCE);
      gUpgradeLowpassSnapshotActive = false;
    }
  }

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

  void UpgradeUIStateMachine::set_picker_open_callback(std::function<void()> cb)
  {
    picker_open_cb_ = std::move(cb);
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
      break;
    }
    case UpgradeFlowState::PickerOpen:
    {
      // Destroy picker entities and resume time
      destroy_picker_entities();
      Time::set_time_scale(1.0f);
      stop_upgrade_lowpass_snapshot();
      break;
    }
    default:
      break;
    }
  }

  void UpgradeUIStateMachine::on_enter(UpgradeFlowState new_state)
  {
    auto& mgr = UpgradeManager::instance();
    //auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();

    switch (new_state)
    {
    case UpgradeFlowState::Available:
    {
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
      start_upgrade_lowpass_snapshot();
      if (picker_open_cb_) picker_open_cb_();
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
    auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();

    // Burst particles tick in any state (they outlive the PickerOpen state)
    if (!burst_particles_.empty())
      update_burst_particles(Time::get_unscaled_fixed_dt());

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

      // Check for Tab key or mouse click on upgrade button
      bool open_picker = Input::is_pressed(KeyCode::Tab);

      if (!open_picker && upgrade_btn_id_ != 0
        && ECS::is_entity_active(ECS::Entity(upgrade_btn_id_)))
      {
        auto* t = ECS::Entity(upgrade_btn_id_).get_component<Components::Transform2D>();
        auto* ui = ECS::Entity(upgrade_btn_id_).get_component<Components::UI>();
        if (t && ui)
        {
          auto vres = Engine::Graphics::get_virtual_resolution();
          glm::vec2 mouse = Input::get_mouse_position();
          float sx = vres.x * ui->anchor.x + t->position.x;
          float sy = vres.y * ui->anchor.y + t->position.y;
          float hw = t->scale.x / 2.0f;
          float hh = t->scale.y / 2.0f;
          if (mouse.x >= sx - hw && mouse.x <= sx + hw &&
            mouse.y >= sy - hh && mouse.y <= sy + hh)
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

      // Don't process input during flip reveal, stamp, or arc animation
      if (!flip_done_) break;
      if (cfg.enable_selection_stamp && stamp_time_ >= 0.0f) break;
      if (arc_active_ || pre_shift_time_ >= 0.0f || smash_time_ >= 0.0f || linger_time_ >= 0.0f) break;

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
            if (cfg.enable_selection_stamp)
            {
              stamp_card_ = 0; stamp_time_ = 0.0f;
            }
            else if (cfg.enable_card_arc)
            {
              start_arc(0);
            }
            else
            {
              handle_card_selection(0);
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
            if (cfg.enable_selection_stamp)
            {
              stamp_card_ = 1; stamp_time_ = 0.0f;
            }
            else if (cfg.enable_card_arc)
            {
              start_arc(1);
            }
            else
            {
              handle_card_selection(1);
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
      // Re-evaluate after applying or skipping.
      // If another upgrade is available, immediately re-roll and open picker.
      if (mgr.can_upgrade())
      {
        if (mgr.roll_cards())
          transition_to(UpgradeFlowState::PickerOpen);
        else
          transition_to(UpgradeFlowState::Idle);
      }
      else
      {
        transition_to(UpgradeFlowState::Idle);
      }
      break;
    }
    }
  }

  // ---- Card animations ----

  void UpgradeUIStateMachine::animate_cards()
  {
    auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();
    float dt = Time::get_unscaled_fixed_dt();
    picker_open_time_ += dt;

    ECS::EntityID card_ids[2] = { card0_id_, card1_id_ };

    // Manually drive card sprite animation using unscaled time (AnimationUpdateSystem
    // uses scaled time which is near-frozen while the picker is open)
    constexpr float card_fps = 12.0f;
    for (int i = 0; i < 2; ++i)
    {
      if (card_ids[i] == 0 || card_frames_[i] <= 1) continue;
      if (auto* atlas = ECS::Entity(card_ids[i]).get_component<Components::TextureAtlas>())
        atlas->drawIndex = static_cast<int>(picker_open_time_ * card_fps) % card_frames_[i];
    }

    // Start from base values each frame; effects accumulate offsets
    float pos_y[2] = { card_base_y_[0], card_base_y_[1] };
    float scale_x[2] = { card_base_sx_[0], card_base_sx_[1] };
    float scale_y[2] = { card_base_sy_[0], card_base_sy_[1] };

    // Entry animation: cards slide down from above and bounce into place
    if (card_intro_active_ && cfg.enable_card_intro)
    {
      const float intro_duration = cfg.intro_duration;
      const float intro_start_offset = cfg.intro_start_offset;
      card_intro_time_ += Time::get_unscaled_fixed_dt();

      float t = std::min(card_intro_time_ / intro_duration, 1.0f);
      // Use smoother ease: start slower, then gentle overshoot and settle.
      float progress = Easing::EaseSineOut(t, 0.0f, 1.0f, 1.0f);

      for (int i = 0; i < 2; ++i)
      {
        float start_y = card_base_y_[i] - intro_start_offset;
        pos_y[i] = start_y + (card_base_y_[i] - start_y) * progress;
      }

      if (t >= 1.0f)
        card_intro_active_ = false;
    }
    else if (card_intro_active_ && !cfg.enable_card_intro)
    {
      // Directly skip intro if support is disabled.
      card_intro_active_ = false;
      for (int i = 0; i < 2; ++i)
      {
        if (card_ids[i] == 0) continue;
        auto* t = ECS::Entity(card_ids[i]).get_component<Components::Transform2D>();
        if (t)
          t->position.y = card_base_y_[i];
      }
    }

    // ---- Mini-card idle wave + smash ripple (applied every frame) ----
    {
      constexpr float two_pi = 6.2831853f;
      constexpr float ss_scale = 2.0f;
      constexpr float mini_w = 128.0f * ss_scale / 6.0f;
      constexpr float mini_h = 224.0f * ss_scale / 6.0f;

      // Advance ripple timer
      if (ripple_time_ >= 0.0f)
        ripple_time_ += dt;

      // Hit-test for mini-card hover
      glm::vec2 mouse = Input::get_mouse_position();

      for (size_t i = 0; i < mini_card_ids_.size(); ++i)
      {
        if (!ECS::is_entity_active(ECS::Entity(mini_card_ids_[i]))) continue;
        auto* mt = ECS::Entity(mini_card_ids_[i]).get_component<Components::Transform2D>();
        if (!mt) continue;

        float y_offset = 0.0f;

        // Sine wave idle bob
        if (cfg.enable_mini_wave)
        {
          y_offset += cfg.mini_wave_amplitude
            * sinf(picker_open_time_ * cfg.mini_wave_speed * two_pi
              + static_cast<float>(i) * cfg.mini_wave_phase);
        }

        // Smash ripple
        if (cfg.enable_smash_ripple && ripple_time_ >= 0.0f)
        {
          float dx = mini_card_base_x_[i] - ripple_origin_x_;
          float dy = mini_card_base_y_[i] - ripple_origin_y_;
          float dist = sqrtf(dx * dx + dy * dy);
          float arrival = dist / cfg.ripple_speed;
          float local_t = ripple_time_ - arrival;
          if (local_t > 0.0f)
          {
            float envelope = expf(-cfg.ripple_decay * local_t);
            y_offset += cfg.ripple_amplitude * sinf(local_t * two_pi * 4.0f) * envelope;
          }
        }

        // Mini-card hover: lift + scale
        bool is_hovered = false;
        if (cfg.enable_mini_hover)
        {
          float cx = mt->position.x;
          float cy = mini_card_base_y_[i];
          float hw = mini_w / 2.0f;
          float hh = mini_h / 2.0f;
          is_hovered = (mouse.x >= cx - hw && mouse.x <= cx + hw &&
            mouse.y >= cy - hh && mouse.y <= cy + hh);
          float target = is_hovered ? 1.0f : 0.0f;
          mini_hover_lerp_[i] += (target - mini_hover_lerp_[i]) * cfg.mini_hover_speed * dt;
          if (mini_hover_lerp_[i] < 0.001f) mini_hover_lerp_[i] = 0.0f;
          if (mini_hover_lerp_[i] > 0.999f) mini_hover_lerp_[i] = 1.0f;

          y_offset += cfg.mini_hover_lift * mini_hover_lerp_[i];
          float s = 1.0f + (cfg.mini_hover_scale - 1.0f) * mini_hover_lerp_[i];
          mt->scale.x = mini_w * s;
          mt->scale.y = mini_h * s;
        }
        else
        {
          mt->scale.x = mini_w;
          mt->scale.y = mini_h;
        }

        mt->position.y = mini_card_base_y_[i] + y_offset;

        // Tilt: hover tilt (mouse-X-based) when hovered, idle tilt when not
        if (is_hovered && cfg.enable_mini_hover)
        {
          float cx = mt->position.x;
          float hw = mt->scale.x / 2.0f;
          float norm_x = (mouse.x - cx) / hw;
          if (norm_x < -1.0f) norm_x = -1.0f;
          if (norm_x > 1.0f) norm_x = 1.0f;
          float target_tilt = cfg.mini_hover_tilt_deg * norm_x;
          mini_card_tilt_[i] += (target_tilt - mini_card_tilt_[i]) * cfg.mini_hover_tilt_speed * dt;
        }
        else if (cfg.enable_mini_tilt)
        {
          float target_tilt = cfg.mini_tilt_amplitude
            * sinf(picker_open_time_ * cfg.mini_tilt_speed * two_pi
              + static_cast<float>(i) * cfg.mini_tilt_phase);
          mini_card_tilt_[i] += (target_tilt - mini_card_tilt_[i]) * cfg.mini_tilt_speed * 6.0f * dt;
        }
        else
        {
          mini_card_tilt_[i] += (0.0f - mini_card_tilt_[i]) * 8.0f * dt;
        }
        mt->rotation = mini_card_tilt_[i];
      }

      // Auto-expire ripple after it's faded enough
      if (ripple_time_ > 3.0f)
        ripple_time_ = -1.0f;
    }

    // ---- Flip Reveal ----
    if (cfg.enable_flip_reveal && !flip_done_)
    {
      const float flip_duration = cfg.flip_duration;
      const float flip_stagger = cfg.flip_stagger;
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
    if (cfg.enable_idle_float && flip_done_)
    {
      const float float_amplitude = cfg.float_amplitude;
      const float float_speed = cfg.float_speed;
      const float float_phase_offset = cfg.float_phase_offset;
      constexpr float two_pi = 6.2831853f;
      for (int i = 0; i < 2; ++i)
      {
        pos_y[i] += float_amplitude * sinf(picker_open_time_ * float_speed * two_pi + i * float_phase_offset);
      }
    }

    // ---- Hover Lift (only after flip completes) ----
    if (cfg.enable_hover_lift && flip_done_)
    {
      const float hover_lift_y = cfg.hover_lift_y;
      const float hover_scale_boost = cfg.hover_scale_boost;
      const float hover_speed = cfg.hover_speed;

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

    if (cfg.enable_idle_tilt && flip_done_)
    {
      const float idle_tilt_amplitude = cfg.idle_tilt_amplitude;
      const float idle_tilt_speed = cfg.idle_tilt_speed;
      const float idle_tilt_phase = cfg.idle_tilt_phase;
      constexpr float two_pi = 6.2831853f;
      for (int i = 0; i < 2; ++i)
      {
        if (hovered_card_ != i)  // only when NOT hovered
        {
          float target = idle_tilt_amplitude
            * sinf(picker_open_time_ * idle_tilt_speed * two_pi + i * idle_tilt_phase);
          card_tilt_[i] += (target - card_tilt_[i]) * cfg.idle_tilt_lerp_speed * dt;
        }
      }
    }

    // ---- Hover Tilt (mouse-X-based tilt on the hovered card) ----
    if (cfg.enable_hover_tilt && flip_done_)
    {
      const float max_tilt_deg = cfg.max_tilt_deg;
      const float tilt_speed = cfg.tilt_speed;

      glm::vec2 mouse = Input::get_mouse_position();
      for (int i = 0; i < 2; ++i)
      {
        if (hovered_card_ == i)
        {
          float norm_x = (mouse.x - card_base_x_[i]) / (card_base_sx_[i] / 2.0f);
          if (norm_x < -1.0f) norm_x = -1.0f;
          if (norm_x > 1.0f) norm_x = 1.0f;
          float target_tilt = max_tilt_deg * norm_x;
          card_tilt_[i] += (target_tilt - card_tilt_[i]) * tilt_speed * dt;
        }
        else if (!cfg.enable_idle_tilt)
        {
          // No idle tilt running — lerp back to 0
          card_tilt_[i] += (0.0f - card_tilt_[i]) * tilt_speed * dt;
        }
      }
    }

    // If both tilt effects are off, snap to 0
    if (!cfg.enable_hover_tilt && !cfg.enable_idle_tilt)
    {
      card_tilt_[0] = 0.0f;
      card_tilt_[1] = 0.0f;
    }

    // ---- Selection Stamp ----
    if (cfg.enable_selection_stamp && stamp_time_ >= 0.0f)
    {
      const float stamp_duration = cfg.stamp_duration;
      const float stamp_squish = cfg.stamp_squish;
      stamp_time_ += dt;

      if (stamp_time_ >= stamp_duration)
      {
        // Stamp finished — start pre-shift, arc, or apply immediately
        stamp_time_ = -1.0f;
        int sc = stamp_card_;
        stamp_card_ = -1;
        if (cfg.enable_card_arc)
        {
          // Populate shift data first (needed for pre-shift and arc target)
          start_arc(sc);  // sets arc state + computes shift targets
          // Check if any mini-cards actually need to shift
          bool needs_shift = false;
          if (cfg.enable_card_stack)
          {
            for (size_t i = 0; i < mini_card_ids_.size(); ++i)
            {
              if (mini_card_row_[i] == arc_target_row_ &&
                mini_card_start_x_[i] != mini_card_target_x_[i])
              {
                needs_shift = true;
                break;
              }
            }
          }
          if (needs_shift)
          {
            // Pause the arc and run pre-shift first
            arc_active_ = false;
            arc_time_ = 0.0f;
            pre_shift_time_ = 0.0f;
            pre_shift_card_ = sc;
          }
          return;
        }
        handle_card_selection(sc);
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

    // ---- Pre-Shift: slide existing mini-cards before arc starts ----
    if (pre_shift_time_ >= 0.0f)
    {
      const float pre_dur = cfg.pre_shift_duration;
      pre_shift_time_ += dt;
      float t = pre_shift_time_ / pre_dur;
      if (t > 1.0f) t = 1.0f;

      float shift_t = Easing::EaseQuadInOut(t, 0.0f, 1.0f, 1.0f);
      for (size_t i = 0; i < mini_card_ids_.size(); ++i)
      {
        if (mini_card_row_[i] != arc_target_row_) continue;
        auto* mt = ECS::Entity(mini_card_ids_[i]).get_component<Components::Transform2D>();
        if (mt)
          mt->position.x = mini_card_start_x_[i] + (mini_card_target_x_[i] - mini_card_start_x_[i]) * shift_t;
      }

      if (t >= 1.0f)
      {
        // Pre-shift done — update base X positions, then start the arc
        for (size_t i = 0; i < mini_card_ids_.size(); ++i)
        {
          if (mini_card_row_[i] == arc_target_row_)
            mini_card_base_x_[i] = mini_card_target_x_[i];
        }
        pre_shift_time_ = -1.0f;
        pre_shift_card_ = -1;
        arc_active_ = true;
        arc_time_ = 0.0f;
      }
      return;
    }

    // ---- Arc-to-Panel Flight ----
    if (arc_active_)
    {
      const float arc_duration = cfg.arc_duration;
      const float arc_peak = cfg.arc_peak;
      constexpr float pi = 3.14159265f;

      arc_time_ += dt;
      float t = arc_time_ / arc_duration;
      if (t > 1.0f) t = 1.0f;

      int ac = arc_card_;
      if (ac >= 0 && ac < 2 && card_ids[ac] != 0)
      {
        // Smooth eased interpolation for X and Y
        float eased = Easing::EaseQuadInOut(t, 0.0f, 1.0f, 1.0f);
        float cx = arc_start_x_ + (arc_end_x_ - arc_start_x_) * eased;
        float cy = arc_start_y_ + (arc_end_y_ - arc_start_y_) * eased;
        // Add parabolic arc (peaks at t=0.5)
        cy += arc_peak * 4.0f * t * (1.0f - t);

        // Scale: shrink from full to mini (accelerating)
        float se = Easing::EaseCubicIn(t, 0.0f, 1.0f, 1.0f);
        float sx = arc_start_sx_ + (arc_end_sx_ - arc_start_sx_) * se;
        float sy = arc_start_sy_ + (arc_end_sy_ - arc_start_sy_) * se;

        // Gentle spin during flight
        float rot = sinf(t * pi) * cfg.arc_spin_deg;

        auto* at = ECS::Entity(card_ids[ac]).get_component<Components::Transform2D>();
        if (at)
        {
          at->position.x = cx;
          at->position.y = cy;
          at->scale.x = sx;
          at->scale.y = sy;
          at->rotation = rot;
        }
      }

      // Fade out the non-selected card
      int other = 1 - ac;
      if (other >= 0 && other < 2 && card_ids[other] != 0)
      {
        auto* oc = ECS::Entity(card_ids[other]).get_component<Components::Color>();
        if (oc)
        {
          float fade_t = Easing::EaseQuadIn(t, 0.0f, 1.0f, 1.0f);
          oc->a = 1.0f - fade_t;
        }
      }

      if (t >= 1.0f)
      {
        arc_active_ = false;
        smash_time_ = 0.0f;  // start smash landing
      }
      return;  // skip normal write during arc
    }

    // ---- Smash Landing ----
    if (smash_time_ >= 0.0f)
    {
      const float smash_duration = cfg.smash_duration;
      smash_time_ += dt;

      int ac = arc_card_;
      if (ac >= 0 && ac < 2 && card_ids[ac] != 0)
      {
        auto* at = ECS::Entity(card_ids[ac]).get_component<Components::Transform2D>();
        if (at)
        {
          at->position.x = arc_end_x_;
          at->position.y = arc_end_y_;
          // EaseBackOut overshoots then settles — from 0.8×mini to 1.0×mini
          float st = smash_time_;
          if (st > smash_duration) st = smash_duration;
          float factor = Easing::EaseBackOut(st, 0.8f, 0.2f, smash_duration);
          at->scale.x = arc_end_sx_ * factor;
          at->scale.y = arc_end_sy_ * factor;
          at->rotation = 0.0f;
        }
      }

      if (smash_time_ >= smash_duration)
      {
        if (cfg.enable_burst_particles)
          spawn_burst_particles(arc_end_x_, arc_end_y_);

        // Trigger ripple from smash impact point
        if (cfg.enable_smash_ripple)
        {
          ripple_time_ = 0.0f;
          ripple_origin_x_ = arc_end_x_;
          ripple_origin_y_ = arc_end_y_;
        }

        // Convert the flying card into a mini-card so it joins the wave
        {
          auto& mgr_ref = UpgradeManager::instance();
          auto& settings_ref = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
          int rolled_idx = mgr_ref.get_rolled_card(arc_card_);

          // Hide the big flying card
          if (arc_card_ >= 0 && arc_card_ < 2)
          {
            ECS::EntityID fly_id = (arc_card_ == 0) ? card0_id_ : card1_id_;
            if (fly_id != 0)
            {
              auto* fc = ECS::Entity(fly_id).get_component<Components::Color>();
              if (fc) fc->a = 0.0f;
            }
          }

          // Spawn a mini-card at the landing spot
          if (rolled_idx >= 0 && rolled_idx < static_cast<int>(settings_ref.upgrades.size()))
          {
            const auto& arc_upg = settings_ref.upgrades[rolled_idx];
            auto mc = ECS::create_entity()
              .add_component<Components::Transform2D>(arc_end_x_, arc_end_y_, 0.0f, arc_end_sx_, arc_end_sy_)
              .add_component<Components::Texture>(Resources::load_texture(arc_upg.card_image))
              .add_component<Components::TextureAtlas>(128, 256, 1, arc_upg.card_frames, arc_upg.card_frames)
              .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
              .add_component<Components::UI>()
              .add_to_layer("upgrade_ui_fg");
            if (auto* atlas = ECS::Entity(mc.get_id()).get_component<Components::TextureAtlas>())
              atlas->drawIndex = 0;

            mini_card_ids_.push_back(mc.get_id());
            mini_card_row_.push_back(arc_target_row_);
            mini_card_base_x_.push_back(arc_end_x_);
            mini_card_base_y_.push_back(arc_end_y_);
            mini_card_start_x_.push_back(arc_end_x_);
            mini_card_target_x_.push_back(arc_end_x_);
            mini_hover_lerp_.push_back(0.0f);
            mini_card_tilt_.push_back(0.0f);
          }
        }

        smash_time_ = -1.0f;
        linger_time_ = 0.0f;  // hold the panel open while particles play
        return;
      }

      // Keep mini-cards at compressed positions during smash
      if (cfg.enable_card_stack && !mini_card_target_x_.empty())
      {
        for (size_t i = 0; i < mini_card_ids_.size(); ++i)
        {
          if (mini_card_row_[i] != arc_target_row_) continue;
          auto* mt = ECS::Entity(mini_card_ids_[i]).get_component<Components::Transform2D>();
          if (mt) mt->position.x = mini_card_target_x_[i];
        }
      }

      return;  // skip normal write during smash
    }

    // ---- Post-smash linger (keep panel visible while particles play) ----
    if (linger_time_ >= 0.0f)
    {
      const float linger_duration = cfg.linger_duration;
      linger_time_ += dt;

      // Keep mini-cards at compressed positions during linger
      if (cfg.enable_card_stack && !mini_card_target_x_.empty())
      {
        for (size_t i = 0; i < mini_card_ids_.size(); ++i)
        {
          if (mini_card_row_[i] != arc_target_row_) continue;
          auto* mt = ECS::Entity(mini_card_ids_[i]).get_component<Components::Transform2D>();
          if (mt) mt->position.x = mini_card_target_x_[i];
        }
      }

      if (linger_time_ >= linger_duration)
      {
        linger_time_ = -1.0f;

        // Apply the flying card and keep the picker open if another upgrade is available.
        int selected = arc_card_;
        arc_card_ = -1;
        if (selected >= 0 && selected < 2)
          UpgradeManager::instance().pick_card(selected);

        auto& mgr = UpgradeManager::instance();
        if (mgr.can_upgrade() && mgr.roll_cards())
        {
          refresh_card_entity_batch();
          return;
        }

        transition_to(UpgradeFlowState::Idle);
        return;
      }
      return;  // keep PickerOpen (panel stays visible)
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
      t->rotation = card_tilt_[i];
    }
  }

  void UpgradeUIStateMachine::refresh_card_entity_batch()
  {
    auto& mgr = UpgradeManager::instance();
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();

    picker_open_time_ = 0.0f;
    card_intro_active_ = true;
    card_intro_time_ = 0.0f;
    flip_done_ = false;
    stamp_time_ = -1.0f;
    stamp_card_ = -1;
    hovered_card_ = -1;
    hover_lerp_[0] = 0.0f;
    hover_lerp_[1] = 0.0f;
    card_tilt_[0] = 0.0f;
    card_tilt_[1] = 0.0f;

    pre_shift_time_ = -1.0f;
    pre_shift_card_ = -1;
    arc_active_ = false;
    arc_time_ = 0.0f;
    smash_time_ = -1.0f;
    linger_time_ = -1.0f;

    for (int i = 0; i < 2; ++i)
    {
      ECS::EntityID card_id = (i == 0) ? card0_id_ : card1_id_;
      if (card_id == 0) continue;

      auto* t = ECS::Entity(card_id).get_component<Components::Transform2D>();
      if (t)
      {
        t->position.x = card_base_x_[i];
        t->position.y = card_base_y_[i] - cfg.intro_start_offset;
        t->scale.x = card_base_sx_[i];
        t->scale.y = card_base_sy_[i];
        t->rotation = 0.0f;
      }

      auto* c = ECS::Entity(card_id).get_component<Components::Color>();
      if (c)
      {
        c->r = 1.0f;
        c->g = 1.0f;
        c->b = 1.0f;
        c->a = 1.0f;
      }

      int rolled_idx = mgr.get_rolled_card(i);
      if (rolled_idx >= 0 && rolled_idx < static_cast<int>(settings.upgrades.size()))
      {
        const auto& upg = settings.upgrades[rolled_idx];
        if (auto* tex = ECS::Entity(card_id).get_component<Components::Texture>())
          *tex = Resources::load_texture(upg.card_image);
        if (auto* atlas = ECS::Entity(card_id).get_component<Components::TextureAtlas>())
          *atlas = Components::TextureAtlas(128, 256, 1, upg.card_frames, upg.card_frames);
        card_frames_[i] = upg.card_frames;
        if (auto* atlas = ECS::Entity(card_id).get_component<Components::TextureAtlas>())
          atlas->drawIndex = 0;
      }
    }
  }

  void UpgradeUIStateMachine::handle_card_selection(int slot)
  {
    if (slot < 0 || slot > 1) return;

    auto& mgr = UpgradeManager::instance();
    mgr.pick_card(slot);

    if (mgr.can_upgrade() && mgr.roll_cards())
    {
      refresh_card_entity_batch();
      return;
    }

    transition_to(UpgradeFlowState::Idle);
  }

  // ---- Picker entity management ----

  void UpgradeUIStateMachine::spawn_picker_entities()
  {
    auto& mgr = UpgradeManager::instance();
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();

    auto rect = Resources::get_rect_texture();

    // ---- Per-card textures (128 x 256 per frame; animated cards have 24 frames = 3072 px wide) ----
    constexpr int   card_w = 128;        // frame width in pixels
    constexpr int   card_h = 256;        // frame height in pixels
    constexpr float card_scale = 2.0f;   // display scale multiplier

    // ---- Panel (UpgradeMenu.png: 192 x 256) ----
    constexpr float panel_img_w = 192.0f;
    constexpr float panel_img_h = 256.0f;
    constexpr float panel_scale = 2.0f;  // display scale multiplier
    // -----------------------------------------------------------------------

    auto panel_tex = Resources::load_texture("images/Upgrades/UpgradeMenu.png");

    // Dimmer — full-screen dark overlay
    auto dimmer = ECS::create_entity()
      .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
      .add_component<Components::Texture>(rect)
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.6f)
      .add_component<Components::UI>()
      .add_to_layer("upgrade_ui_bg");
    dimmer_id_ = dimmer.get_id();

    // Layout sizing — derived from image sizes and scale factors above
    constexpr float card_display_w = static_cast<float>(card_w) * card_scale;
    constexpr float card_display_h = static_cast<float>(card_h) * card_scale;
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
      .add_to_layer("upgrade_ui_fg");
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

    // Two-pass layout: first count cards per row, then position with stacking
    for (size_t i = 0; i < settings.upgrades.size(); ++i)
    {
      const auto& upg = settings.upgrades[i];
      int level = mgr.get_upgrade_level(upg.id);
      if (level <= 0) continue;
      int cat = get_category(upg.stat_name);
      if (cat >= 0) row_card_count[cat] += level;
    }

    // Compute effective spacing per row.
    // When stacking: the last card sits at the Nth full-spacing slot.
    // All previous cards are compressed together with stack_offset.
    // When not stacking: all cards evenly spaced.
    float row_base_spacing = mini_w + mini_gap;  // full spacing

    // Reset counts for positioning pass
    int row_pos[4] = { 0, 0, 0, 0 };

    for (size_t i = 0; i < settings.upgrades.size(); ++i)
    {
      const auto& upg = settings.upgrades[i];
      int level = mgr.get_upgrade_level(upg.id);
      if (level <= 0) continue;

      int cat = get_category(upg.stat_name);
      if (cat < 0) continue;

      for (int lv = 0; lv < level; ++lv)
      {
        int idx_in_row = row_pos[cat];
        int total_in_row = row_card_count[cat];
        //bool is_last = (idx_in_row == total_in_row - 1);

        float x;
        if (!cfg.enable_card_stack || total_in_row <= 4)
        {
          // 4 or fewer — normal full spacing
          x = panel_left + row_start_x + mini_w / 2.0f
            + idx_in_row * row_base_spacing;
        }
        else
        {
          // >4: evenly distribute so last card lands at slot 3 position
          float span = 3.0f * row_base_spacing;
          float step = span / (total_in_row - 1);
          x = panel_left + row_start_x + mini_w / 2.0f
            + idx_in_row * step;
        }
        float y = panel_top + row_y[cat];

        auto card_tex = Resources::load_texture(upg.card_image);
        auto mc = ECS::create_entity()
          .add_component<Components::Transform2D>(x, y, 0.0f, mini_w, mini_h)
          .add_component<Components::Texture>(card_tex)
          .add_component<Components::TextureAtlas>(card_w, card_h, 1, upg.card_frames, upg.card_frames)
          .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
          .add_component<Components::UI>()
          .add_to_layer("upgrade_ui_fg");
        if (auto* atlas = ECS::Entity(mc.get_id()).get_component<Components::TextureAtlas>())
          atlas->drawIndex = 0;
        mini_card_ids_.push_back(mc.get_id());
        mini_card_row_.push_back(cat);
        mini_card_base_x_.push_back(x);
        mini_card_base_y_.push_back(y);
        mini_hover_lerp_.push_back(0.0f);
        mini_card_tilt_.push_back(0.0f);

        ++row_pos[cat];
      }
    }

    // Card 0 — center
    int idx0 = mgr.get_rolled_card(0);
    const auto* upg0 = (idx0 >= 0 && idx0 < static_cast<int>(settings.upgrades.size()))
      ? &settings.upgrades[idx0] : nullptr;
    auto c0tex = Resources::load_texture(upg0 ? upg0->card_image : "images/Upgrades/Card_Basic_Dmg.png");
    int c0frames = upg0 ? upg0->card_frames : 1;
    card_frames_[0] = c0frames;

    auto c0 = ECS::create_entity()
      .add_component<Components::Transform2D>(card0_cx, center_y, 0.0f, card_display_w, card_display_h)
      .add_component<Components::Texture>(c0tex)
      .add_component<Components::TextureAtlas>(card_w, card_h, 1, c0frames, c0frames)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("upgrade_ui_fg");
    if (auto* atlas = ECS::Entity(c0.get_id()).get_component<Components::TextureAtlas>())
      atlas->drawIndex = 0;
    card0_id_ = c0.get_id();

    // Card 1 — right
    int idx1 = mgr.get_rolled_card(1);
    const auto* upg1 = (idx1 >= 0 && idx1 < static_cast<int>(settings.upgrades.size()))
      ? &settings.upgrades[idx1] : nullptr;
    auto c1tex = Resources::load_texture(upg1 ? upg1->card_image : "images/Upgrades/Card_Basic_Dmg.png");
    int c1frames = upg1 ? upg1->card_frames : 1;
    card_frames_[1] = c1frames;

    auto c1 = ECS::create_entity()
      .add_component<Components::Transform2D>(card1_cx, center_y, 0.0f, card_display_w, card_display_h)
      .add_component<Components::Texture>(c1tex)
      .add_component<Components::TextureAtlas>(card_w, card_h, 1, c1frames, c1frames)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::UI>()
      .add_to_layer("upgrade_ui_fg");
    if (auto* atlas = ECS::Entity(c1.get_id()).get_component<Components::TextureAtlas>())
      atlas->drawIndex = 0;
    card1_id_ = c1.get_id();

    // ---- Store base values and reset animation state ----
    card_base_x_[0] = card0_cx;    card_base_y_[0] = center_y;
    card_base_sx_[0] = card_display_w; card_base_sy_[0] = card_display_h;
    card_base_x_[1] = card1_cx;    card_base_y_[1] = center_y;
    card_base_sx_[1] = card_display_w; card_base_sy_[1] = card_display_h;

    picker_open_time_ = 0.0f;
    card_intro_active_ = true;
    card_intro_time_ = 0.0f;
    hovered_card_ = -1;

    // Place cards at intro start position to avoid a one-frame snap to final.
    //auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();
    float start_offset = cfg.intro_start_offset;
    if (card0_id_ != 0)
    {
      if (auto* t = ECS::Entity(card0_id_).get_component<Components::Transform2D>())
        t->position.y = card_base_y_[0] - start_offset;
    }
    if (card1_id_ != 0)
    {
      if (auto* t = ECS::Entity(card1_id_).get_component<Components::Transform2D>())
        t->position.y = card_base_y_[1] - start_offset;
    }
    hover_lerp_[0] = 0.0f;
    hover_lerp_[1] = 0.0f;
    card_tilt_[0] = 0.0f;
    card_tilt_[1] = 0.0f;
    stamp_time_ = -1.0f;
    stamp_card_ = -1;
    arc_active_ = false;
    arc_time_ = 0.0f;
    smash_time_ = -1.0f;
    arc_card_ = -1;
    linger_time_ = -1.0f;

    if (cfg.enable_flip_reveal)
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
    mini_card_row_.clear();
    mini_card_start_x_.clear();
    mini_card_target_x_.clear();
    mini_card_base_x_.clear();
    mini_card_base_y_.clear();
    mini_hover_lerp_.clear();
    mini_card_tilt_.clear();
    destroy_if_active(card0_id_);
    destroy_if_active(card1_id_);
    // burst_particles_ intentionally NOT destroyed here — they outlive the picker
  }

  // ---- Reset ----

  void UpgradeUIStateMachine::reset()
  {
    stop_upgrade_lowpass_snapshot();

    if (state_ == UpgradeFlowState::PickerOpen)
    {
      destroy_picker_entities();
      Time::set_time_scale(1.0f);
    }
    state_ = UpgradeFlowState::Idle;
    picked_slot_ = -1;
    arc_active_ = false;
    arc_time_ = 0.0f;
    smash_time_ = -1.0f;
    arc_card_ = -1;
    linger_time_ = -1.0f;
    pre_shift_time_ = -1.0f;
    pre_shift_card_ = -1;
    ripple_time_ = -1.0f;
    card_intro_active_ = false;
    card_intro_time_ = 0.0f;
    // Also clean up any lingering burst particles on hard reset
    {
      auto destroy_if_active = [](ECS::EntityID& id)
        {
          if (id != 0 && ECS::is_entity_active(ECS::Entity(id)))
            ECS::destroy_entity(ECS::Entity(id));
          id = 0;
        };
      for (auto& p : burst_particles_)
        destroy_if_active(p.id);
      burst_particles_.clear();
    }
  }

  // ---- Arc helpers ----

  void UpgradeUIStateMachine::compute_arc_target(int slot, float& out_x, float& out_y)
  {
    auto& mgr = UpgradeManager::instance();
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();

    // Same layout constants as spawn_picker_entities
    constexpr float ss_scale = 2.0f;
    constexpr float card_display_w = 128.0f * ss_scale;  // 256
    //constexpr float card_display_h = 224.0f * ss_scale;  // 448
    constexpr float mini_w = card_display_w / 6.0f;
    //constexpr float mini_h = card_display_h / 6.0f;
    constexpr float mini_gap = 2.0f;
    constexpr float panel_img_w = 192.0f;
    constexpr float panel_scale = 2.0f;
    constexpr float panel_display_w = panel_img_w * panel_scale;
    constexpr float panel_display_h = 256.0f * panel_scale;
    constexpr float gap = 10.0f;
    constexpr float row_start_x = 150.0f;
    constexpr float row_start_y = 142.0f;
    constexpr float row_height = 94.0f;
    constexpr float row_y[4] = { row_start_y, row_start_y + row_height,
      row_start_y + 2.0f * row_height, row_start_y + 3.0f * row_height };

    float group_w = panel_display_w + gap + card_display_w + gap + card_display_w;
    float group_left = (1600.0f - group_w) / 2.0f;
    float panel_cx = group_left + panel_display_w / 2.0f;
    float panel_left = panel_cx - panel_display_w / 2.0f;
    float panel_top = 450.0f - panel_display_h / 2.0f;

    // Category lookup (same as spawn_picker_entities)
    auto get_category = [](const std::string& stat_name) -> int {
      if (stat_name == "attack_damage")                                   return 0;
      if (stat_name == "uppercut_damage" || stat_name == "uppercut_cooldown")  return 1;
      if (stat_name == "downsmash_damage" || stat_name == "downsmash_cooldown") return 2;
      if (stat_name == "dash_cooldown")                                   return 3;
      return -1;
      };

    // Count existing cards per row (current levels BEFORE this pick is applied)
    int row_card_count[4] = { 0, 0, 0, 0 };
    for (size_t i = 0; i < settings.upgrades.size(); ++i)
    {
      const auto& upg = settings.upgrades[i];
      int level = mgr.get_upgrade_level(upg.id);
      if (level <= 0) continue;
      int cat = get_category(upg.stat_name);
      if (cat >= 0) row_card_count[cat] += level;
    }

    // Determine which row the picked card belongs to
    int idx = mgr.get_rolled_card(slot);
    int cat = -1;
    if (idx >= 0 && idx < static_cast<int>(settings.upgrades.size()))
      cat = get_category(settings.upgrades[idx].stat_name);

    if (cat < 0)
    {
      // Fallback: aim at panel center
      out_x = panel_cx;
      out_y = 450.0f;
      return;
    }

    // The new card will be at position row_card_count[cat] (0-indexed)
    int new_count = row_card_count[cat] + 1;
    auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();

    float full_spacing = mini_w + mini_gap;
    int target_slot;
    if (cfg.enable_card_stack && new_count > 4)
    {
      // Stacking: last card always at the 4th slot (index 3)
      target_slot = 3;
    }
    else
    {
      target_slot = new_count - 1;
    }
    out_x = panel_left + row_start_x + mini_w / 2.0f + target_slot * full_spacing;
    out_y = panel_top + row_y[cat];
  }

  void UpgradeUIStateMachine::start_arc(int card_slot)
  {
    constexpr float ss_scale = 2.0f;
    constexpr float mini_w = 128.0f * ss_scale / 6.0f;
    constexpr float mini_h = 224.0f * ss_scale / 6.0f;
    constexpr float mini_gap = 2.0f;
    constexpr float row_base_spacing = mini_w + mini_gap;

    arc_active_ = true;
    arc_time_ = 0.0f;
    arc_card_ = card_slot;
    arc_start_x_ = card_base_x_[card_slot];
    arc_start_y_ = card_base_y_[card_slot];
    arc_start_sx_ = card_base_sx_[card_slot];
    arc_start_sy_ = card_base_sy_[card_slot];
    arc_end_sx_ = mini_w;
    arc_end_sy_ = mini_h;
    compute_arc_target(card_slot, arc_end_x_, arc_end_y_);

    // Compute mini-card shift: figure out which row the arc targets,
    // then compute compressed positions for existing cards in that row.
    auto& mgr = UpgradeManager::instance();
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();

    auto get_category = [](const std::string& stat_name) -> int {
      if (stat_name == "attack_damage")                                   return 0;
      if (stat_name == "uppercut_damage" || stat_name == "uppercut_cooldown")  return 1;
      if (stat_name == "downsmash_damage" || stat_name == "downsmash_cooldown") return 2;
      if (stat_name == "dash_cooldown")                                   return 3;
      return -1;
      };

    int idx = mgr.get_rolled_card(card_slot);
    arc_target_row_ = -1;
    if (idx >= 0 && idx < static_cast<int>(settings.upgrades.size()))
      arc_target_row_ = get_category(settings.upgrades[idx].stat_name);

    // Count cards in the target row (current state, before pick is applied)
    int row_count = 0;
    for (size_t i = 0; i < mini_card_ids_.size(); ++i)
      if (mini_card_row_[i] == arc_target_row_) ++row_count;

    // After the new card lands, total becomes row_count + 1.
    int new_total = row_count + 1;

    // Store current X positions and compute compressed targets.
    mini_card_start_x_.resize(mini_card_ids_.size());
    mini_card_target_x_.resize(mini_card_ids_.size());

    for (size_t i = 0; i < mini_card_ids_.size(); ++i)
    {
      auto* tr = ECS::Entity(mini_card_ids_[i]).get_component<Components::Transform2D>();
      mini_card_start_x_[i] = tr ? tr->position.x : 0.0f;

      if (cfg.enable_card_stack && mini_card_row_[i] == arc_target_row_ && new_total > 4)
      {
        // >4: evenly redistribute so incoming card (at slot 3) and all
        // existing cards are equally spaced.
        int row_idx = 0;
        for (size_t j = 0; j < i; ++j)
          if (mini_card_row_[j] == arc_target_row_) ++row_idx;

        float span = 3.0f * row_base_spacing;
        float step = span / (new_total - 1);
        float base_x = arc_end_x_ - 3.0f * row_base_spacing;
        mini_card_target_x_[i] = base_x + row_idx * step;
      }
      else
      {
        // Not in the target row or stacking disabled — no movement
        mini_card_target_x_[i] = mini_card_start_x_[i];
      }
    }
  }

  // ---- Burst particle helpers ----

  void UpgradeUIStateMachine::spawn_burst_particles(float x, float y)
  {
    auto& cfg = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable();
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> angle_dist(cfg.particle_angle_min, cfg.particle_angle_max);
    std::uniform_real_distribution<float> speed_dist(cfg.particle_speed_min, cfg.particle_speed_max);
    std::uniform_real_distribution<float> life_dist(cfg.particle_life_min, cfg.particle_life_max);
    std::uniform_int_distribution<int>    count_dist(cfg.particle_count_min, cfg.particle_count_max);
    std::uniform_int_distribution<int>    color_dist(0, 6);

    // Vivid palette: gold, orange, red, magenta, cyan, green, white
    static const float palette[7][3] = {
      {1.0f, 0.85f, 0.0f},  // gold
      {1.0f, 0.45f, 0.0f},  // orange
      {1.0f, 0.15f, 0.15f}, // red
      {1.0f, 0.0f,  1.0f},  // magenta
      {0.0f, 0.9f,  1.0f},  // cyan
      {0.2f, 1.0f,  0.2f},  // green
      {1.0f, 1.0f,  1.0f},  // white
    };

    auto rect = Resources::get_rect_texture();
    int count = count_dist(rng);

    for (int i = 0; i < count; ++i)
    {
      float angle = angle_dist(rng);
      float speed = speed_dist(rng);
      float life = life_dist(rng);
      const float* c = palette[color_dist(rng)];

      auto e = ECS::create_entity()
        .add_component<Components::Transform2D>(x, y, 0.0f, cfg.particle_size, cfg.particle_size)
        .add_component<Components::Texture>(rect)
        .add_component<Components::Color>(c[0], c[1], c[2], 1.0f)
        .add_component<Components::UI>()
        .add_to_layer("upgrade_ui_fg");

      BurstParticle bp;
      bp.id = e.get_id();
      bp.vx = cosf(angle) * speed;
      bp.vy = sinf(angle) * speed;
      bp.r = c[0]; bp.g = c[1]; bp.b = c[2];
      bp.life = life;
      bp.max_life = life;
      burst_particles_.push_back(bp);
    }
  }

  void UpgradeUIStateMachine::update_burst_particles(float dt)
  {
    for (auto it = burst_particles_.begin(); it != burst_particles_.end(); )
    {
      it->life -= dt;
      if (it->life <= 0.0f)
      {
        if (it->id != 0 && ECS::is_entity_active(ECS::Entity(it->id)))
          ECS::destroy_entity(ECS::Entity(it->id));
        it = burst_particles_.erase(it);
        continue;
      }

      const float gravity = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable().particle_gravity;
      it->vy += gravity * dt;

      const float particle_size = FileIO::SettingsManager<FileIO::CardAnimSettings>::get_mutable().particle_size;

      float t = 1.0f - (it->life / it->max_life); // 0 → 1 over lifetime
      auto* tr = ECS::Entity(it->id).get_component<Components::Transform2D>();
      if (tr)
      {
        tr->position.x += it->vx * dt;
        tr->position.y += it->vy * dt;
        float shrink = 1.0f - t;  // 1 → 0
        tr->scale.x = particle_size * shrink;
        tr->scale.y = particle_size * shrink;
      }
      auto* col = ECS::Entity(it->id).get_component<Components::Color>();
      if (col)
      {
        col->r = it->r;
        col->g = it->g;
        col->b = it->b;
        col->a = 1.0f;
      }

      ++it;
    }
  }

} // namespace Engine
