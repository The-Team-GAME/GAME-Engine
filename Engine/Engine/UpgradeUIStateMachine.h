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

    // Animation effect toggles — can be changed at runtime (e.g. from ImGui panel).
    bool enable_flip_reveal = true;
    bool enable_idle_float = true;
    bool enable_hover_lift = true;
    bool enable_selection_stamp = true;

  private:
    UpgradeUIStateMachine() = default;

    void transition_to(UpgradeFlowState new_state);
    void on_exit(UpgradeFlowState old_state);
    void on_enter(UpgradeFlowState new_state);

    void spawn_picker_entities();
    void destroy_picker_entities();
    void animate_cards();

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
    float stamp_time_ = -1.0f;
    int   stamp_card_ = -1;
    bool  flip_done_ = false;
  };

} // namespace Engine
