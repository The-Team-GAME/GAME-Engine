//-----------------------------------------------------------------------------------
// file:    StateMachineComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for state machine system. Provides a robust
//          state machine implementation with lifecycle callbacks (on_enter, on_update,
//          on_exit) and deferred state transitions to prevent mid-update bugs.
//          Supports state validation, history tracking, and clean entity-based APIs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../ECS/Macros.h"
#include "../Math/Vector2D.h"
#include "GraphicsComponents.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <iostream>

namespace Components
{
  // ============================================================================
  // State Animation Definition
  // ============================================================================

  struct StateAnimation {
    Texture texture;
    TextureAtlas texture_atlas;
    Animation animation;
    std::optional<Math::Vec2D> scale; // Optional scale override for this state, for entities that have different sizes per animation

    // Note: Animation looping is handled by the AnimationUpdateSystem using fmod
    // If you need non-looping animations, implement custom logic in state callbacks
  };

  // ============================================================================
  // State Definition
  // ============================================================================

  struct State {
    std::string name;

    // Lifecycle callbacks - all receive the entity they're attached to
    std::function<void(Engine::ECS::Entity&)> on_enter;
    std::function<void(Engine::ECS::Entity&)> on_update;
    std::function<void(Engine::ECS::Entity&)> on_exit;

    // Optional animation data for this state
    std::optional<StateAnimation> animation;

    State() = default;

    State(const std::string& state_name,
      std::function<void(Engine::ECS::Entity&)> enter_callback = nullptr,
      std::function<void(Engine::ECS::Entity&)> update_callback = nullptr,
      std::function<void(Engine::ECS::Entity&)> exit_callback = nullptr,
      std::optional<StateAnimation> state_animation = std::nullopt)
      : name(state_name)
      , on_enter(enter_callback)
      , on_update(update_callback)
      , on_exit(exit_callback)
      , animation(state_animation)
    {
    }
  };

  // ============================================================================
  // StateMachine Component
  // ============================================================================

  class StateMachine {
  private:
    std::unordered_map<std::string, State> states_;
    std::string current_state_name_;
    std::string pending_state_name_;  // For deferred transitions
    bool has_pending_transition_ = false;
    bool is_first_frame_ = false;     // Track if we just entered current state
    std::vector<std::string> state_history_; // Track previous states

    Engine::ECS::Entity* owner_entity_ = nullptr;  // Reference to owning entity

  public:
    StateMachine() = default;

    // ========================================================================
    // State Management
    // ========================================================================

    /// Add a state to the state machine
    void add_state(const State& state) {
      if (states_.find(state.name) != states_.end()) {
        std::cout << "Warning: State '" << state.name << "' already exists in state machine. Overwriting." << std::endl;
      }
      states_[state.name] = state;
    }

    /// Set the initial state (must be called before first update)
    void set_initial_state(const std::string& state_name) {
      if (states_.find(state_name) == states_.end()) {
        std::cout << "Error: Cannot set initial state '" << state_name << "' - state does not exist" << std::endl;
        return;
      }

      current_state_name_ = state_name;
      is_first_frame_ = true;
    }

    /// Request a state change (deferred until end of current update)
    void change_state(const std::string& new_state) {
      if (states_.find(new_state) == states_.end()) {
        std::cout << "Error: Cannot change to state '" << new_state << "' - state does not exist" << std::endl;
        return;
      }

      // Mark transition as pending - will execute after current update completes
      pending_state_name_ = new_state;
      has_pending_transition_ = true;
    }

    /// Force an immediate state change (use with caution - can cause issues if called during update)
    void force_immediate_state_change(const std::string& new_state) {
      if (states_.find(new_state) == states_.end()) {
        std::cout << "Error: Cannot change to state '" << new_state << "' - state does not exist" << std::endl;
        return;
      }

      if (new_state == current_state_name_) {
        return; // Already in this state
      }

      execute_state_transition(new_state);
    }

    /// Revert to the previous state in history
    void revert_to_previous_state() {
      if (state_history_.empty()) {
        std::cout << "Warning: No previous state to revert to" << std::endl;
        return;
      }

      std::string previous_state = state_history_.back();
      state_history_.pop_back(); // Remove it so we don't add it again
      change_state(previous_state);
    }

    // ========================================================================
    // Update (called by StateMachineSystem)
    // ========================================================================


    /// Update the current state and handle deferred transitions
    /// Should be called once per frame by the StateMachineSystem
    void update(Engine::ECS::Entity& entity) {
      // Store entity reference
      owner_entity_ = &entity;

      if (current_state_name_.empty()) {
        std::cout << "Warning: StateMachine has no current state set. Call set_initial_state() first." << std::endl;
        return;
      }

      // Call on_enter if this is the first frame after entering this state
      if (is_first_frame_) {
        auto& current_state = states_[current_state_name_];
        if (current_state.on_enter) {
          current_state.on_enter(entity);
        }
        is_first_frame_ = false;
      }

      // Execute current state's update
      auto& current_state = states_[current_state_name_];
      if (current_state.on_update) {
        current_state.on_update(entity);
      }

      // After update completes, apply any pending state transition
      if (has_pending_transition_) {
        execute_state_transition(pending_state_name_);
        has_pending_transition_ = false;
      }
    }

    // ========================================================================
    // Queries
    // ========================================================================

    /// Get the name of the current state
    const std::string& get_current_state() const {
      return current_state_name_;
    }

    /// Check if currently in a specific state
    bool is_in_state(const std::string& state_name) const {
      return current_state_name_ == state_name;
    }

    /// Check if this is the first frame after entering the current state
    bool is_first_frame() const {
      return is_first_frame_;
    }

    /// Check if a state exists in this state machine
    bool has_state(const std::string& state_name) const {
      return states_.find(state_name) != states_.end();
    }

    /// Get the state history (for debugging)
    const std::vector<std::string>& get_state_history() const {
      return state_history_;
    }

  private:
    // ========================================================================
    // Internal State Transition Logic
    // ========================================================================
    void execute_state_transition(const std::string& new_state) {
      if (new_state == current_state_name_) {
        return; // Already in this state
      }

      if (!owner_entity_) {
        std::cout << "Error: Cannot execute state transition - owner entity is null" << std::endl;
        return;
      }

      // Exit current state
      if (!current_state_name_.empty()) {
        auto& old_state = states_[current_state_name_];
        if (old_state.on_exit) {
          old_state.on_exit(*owner_entity_);
        }

        // Add to history (limit history size to prevent unbounded growth)
        state_history_.push_back(current_state_name_);
        if (state_history_.size() > 10) { // Keep last 10 states
          state_history_.erase(state_history_.begin());
        }
      }

      // Transition to new state
      current_state_name_ = new_state;
      is_first_frame_ = true; // Mark that we need to call on_enter next update

      // Apply state animation if it exists
      auto& new_state_obj = states_[new_state];
      if (new_state_obj.animation.has_value()) {
        apply_state_animation(new_state_obj.animation.value());
      }
    }

    // Apply the animation data to the entity
    void apply_state_animation(const StateAnimation& anim) {
      if (!owner_entity_) return;

      // Update Texture component
      if (auto* texture = owner_entity_->get_component<Texture>()) {
        *texture = anim.texture;
      }
      else {
        owner_entity_->add_component<Texture>(anim.texture);
      }

      // Update TextureAtlas component
      if (auto* atlas = owner_entity_->get_component<TextureAtlas>()) {
        *atlas = anim.texture_atlas;
      }
      else {
        owner_entity_->add_component<TextureAtlas>(anim.texture_atlas);
      }

      // Update Animation component and reset it to start from the beginning
      if (auto* animation = owner_entity_->get_component<Animation>()) {
        *animation = anim.animation;
        animation->startTime = Engine::Time::get_time(); // Reset animation start time
      }
      else {
        Animation new_anim = anim.animation;
        new_anim.startTime = Engine::Time::get_time(); // Set current time as start
        owner_entity_->add_component<Animation>(new_anim);
      }

      // Apply optional scale override if specified
      if (anim.scale.has_value()) {
        if (auto* transform = owner_entity_->get_component<Transform2D>()) {
          transform->scale = anim.scale.value();
        }
      }
    }
  };

} // namespace Components

REFLECT_FIELDS(Components::StateMachine)
