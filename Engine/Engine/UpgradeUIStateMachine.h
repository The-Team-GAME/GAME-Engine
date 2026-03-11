//-----------------------------------------------------------------------------------
// file:    UpgradeUIStateMachine.h
// project: Monkey Stole My GPA
//
// brief:   Dedicated state machine for the entity-based upgrade picker UI.
//          Manages the full upgrade flow: idle -> available -> rolling -> picker
//          -> applying/skipping. Spawns and destroys UI entities (colored rects)
//          for the card-pick overlay. Called once per frame from GameplayState.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <vector>
#include "EngineExport.h"
#include "ECS/Types.h"

namespace Engine
{
  // Lightweight particle used for the smash-landing burst effect.
  struct BurstParticle
  {
    ECS::EntityID id = 0;
    float vx = 0.0f, vy = 0.0f;  // velocity (px/s)
    float r = 1.0f, g = 1.0f, b = 1.0f;  // spawn color
    float life = 0.0f;            // remaining lifetime (s)
    float max_life = 0.0f;        // initial lifetime (s)
  };
  enum class UpgradeFlowState
  {
    Idle,       // Points < threshold or all maxed. Button hidden.
    Available,  // can_upgrade() true. Button visible, waiting for input.
    Rolling,    // Cards being rolled (instant transition, hook for future anim).
    PickerOpen, // Picker UI entities visible, time paused, waiting for pick/skip.
    Applying,   // Card picked — applying upgrade, then re-evaluate.
    Skipping    // Cards skipped — deducting points, then re-evaluate.
  };

  class UpgradeUIStateMachine
  {
  public:
    ENGINE_API static UpgradeUIStateMachine& instance();
    ENGINE_API static void destroy_instance();

    // Call once per frame (from GameplayState::update).
    ENGINE_API void update();

    // Reset to Idle, destroy any picker entities, resume time.
    ENGINE_API void reset();

    // Current state (read-only, for debug display).
    ENGINE_API UpgradeFlowState get_state() const;

    // Tell the FSM which entity is the upgrade button (call after button creation).
    ENGINE_API void set_upgrade_button(ECS::EntityID id);

  private:
    UpgradeUIStateMachine() = default;

    void transition_to(UpgradeFlowState new_state);
    void on_exit(UpgradeFlowState old_state);
    void on_enter(UpgradeFlowState new_state);

    void spawn_picker_entities();
    void destroy_picker_entities();
    void animate_cards();

    // Compute the target mini-card slot position on the panel for a picked card.
    void compute_arc_target(int slot, float& out_x, float& out_y);

    // Start the arc-to-panel animation for the given card slot.
    void start_arc(int card_slot);

    UpgradeFlowState state_ = UpgradeFlowState::Idle;

    // Upgrade button entity (created externally, FSM controls its alpha)
    ECS::EntityID upgrade_btn_id_ = 0;

    // Picker UI entities (owned by this FSM)
    ECS::EntityID dimmer_id_ = 0;
    ECS::EntityID panel_id_ = 0;
    ECS::EntityID card0_id_ = 0;
    ECS::EntityID card1_id_ = 0;
    std::vector<ECS::EntityID> mini_card_ids_;

    int picked_slot_ = -1;

    // Animation state
    float card_base_x_[2] = {};
    float card_base_y_[2] = {};
    float card_base_sx_[2] = {};
    float card_base_sy_[2] = {};
    float picker_open_time_ = 0.0f;
    int   hovered_card_ = -1;
    float hover_lerp_[2] = {};
    float card_tilt_[2] = {};   // current Z-rotation (degrees) for tilt effects
    float stamp_time_ = -1.0f;
    int   stamp_card_ = -1;
    bool  flip_done_ = false;

    // Arc-to-panel animation state
    bool  arc_active_ = false;
    float arc_time_ = 0.0f;
    int   arc_card_ = -1;         // which card (0 or 1) is flying
    float arc_start_x_ = 0.0f, arc_start_y_ = 0.0f;
    float arc_end_x_ = 0.0f, arc_end_y_ = 0.0f;
    float arc_start_sx_ = 0.0f, arc_start_sy_ = 0.0f;
    float arc_end_sx_ = 0.0f, arc_end_sy_ = 0.0f;
    float smash_time_ = -1.0f;   // landing bounce timer (-1 = inactive)
    float linger_time_ = -1.0f;  // post-smash hold before transitioning (-1 = inactive)

    // Mini-card shift during arc (existing cards compress to make room)
    std::vector<float> mini_card_start_x_;  // original X per mini-card
    std::vector<float> mini_card_target_x_; // compressed X per mini-card
    std::vector<float> mini_card_base_x_;   // resting X per mini-card
    std::vector<float> mini_card_base_y_;   // resting Y per mini-card
    std::vector<int>   mini_card_row_;       // category row per mini-card
    int arc_target_row_ = -1;               // which row the arc card targets

    // Pre-shift phase: existing cards slide to make room before arc starts
    float pre_shift_time_ = -1.0f;
    int   pre_shift_card_ = -1;

    // Smash ripple state
    float ripple_time_ = -1.0f;  // -1 = inactive
    float ripple_origin_x_ = 0.0f;
    float ripple_origin_y_ = 0.0f;

    // Pixel particle burst on smash impact
    std::vector<BurstParticle> burst_particles_;
    void spawn_burst_particles(float x, float y);
    void update_burst_particles(float dt);
  };

} // namespace Engine
