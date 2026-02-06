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
#include "../FunctionRegistry.h"
#include "GraphicsComponents.h"
#include "CollisionComponents.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include "../Logger.h"
#include <queue>
#include "SpatialComponents.h"
#include "../Resources.h"
#include "../Audio/AudioPlayer.h"
#include "../Audio/AudioPlayerInternal.h"
#include "AudioComponents.h"

namespace Components
{
  // ============================================================================
  // State Audio Definition
  // ============================================================================

  struct StateAudio {
    Components::Sound sound;
    f32 volume = 1.0f;
    bool loop = false;
    bool stop_on_exit = true;
  };

  // ============================================================================
  // State Animation Definition
  // ============================================================================

  struct ColliderData {
    Components::ColliderType type;
    f32 width;
    f32 height;
    Math::Vec2D offset;
    std::vector<Math::Vec2D> vertices; // For polygon colliders (local space)

    ColliderData()
      : type(Components::ColliderType::AABB)
      , width(0.0f)
      , height(0.0f)
      , offset(0.0f, 0.0f)
      , vertices()
    {
    }
  };

  struct StateAnimation {
    Texture texture;
    TextureAtlas texture_atlas;
    Animation animation;
    std::optional<Math::Vec2D> scale; // Optional scale override for this state
    std::optional<ColliderData> collider_data; // Optional collider size override
  };

  // ============================================================================
  // State Definition
  // ============================================================================

  struct State {
    std::string name;

    // Callback names (serializable) - empty string means no callback
    Engine::SerializableFunction on_enter_callback;
    Engine::SerializableFunction on_update_callback;
    Engine::SerializableFunction on_exit_callback;

    // Optional animation data for this state
    std::optional<StateAnimation> animation;
    // Optional audio data for this state (OLD SYSTEM - kept for backwards compatibility)
    std::optional<StateAudio> audio;
    // Optional FMOD audio data for this state (NEW SYSTEM - preferred)
    std::optional<FMODStateAudio> fmod_audio;

    State() = default;

    State(const std::string& state_name,
      const std::string& enter_callback = "",
      const std::string& update_callback = "",
      const std::string& exit_callback = "",
      std::optional<StateAnimation> state_animation = std::nullopt,
      std::optional<StateAudio> state_audio = std::nullopt,
      std::optional<FMODStateAudio> fmod_state_audio = std::nullopt)
      : name(state_name)
      , on_enter_callback(enter_callback)
      , on_update_callback(update_callback)
      , on_exit_callback(exit_callback)
      , animation(state_animation)
      , audio(state_audio)
      , fmod_audio(fmod_state_audio)
    {
    }

    // Helper methods to invoke callbacks (looks them up in registry)
    void invoke_on_enter(Engine::ECS::Entity entity) const {
      if (!on_enter_callback.empty()) {
        auto callback = on_enter_callback.get<void(Engine::ECS::Entity)>();
        if (callback) {
          callback(entity);
        }
      }
    }

    void invoke_on_update(Engine::ECS::Entity entity) const {
      if (!on_update_callback.empty()) {
        auto callback = on_update_callback.get<void(Engine::ECS::Entity)>();
        if (callback) {
          callback(entity);
        }
      }
    }

    void invoke_on_exit(Engine::ECS::Entity entity) const {
      if (!on_exit_callback.empty()) {
        auto callback = on_exit_callback.get<void(Engine::ECS::Entity)>();
        if (callback) {
          callback(entity);
        }
      }
    }
  };

  // ============================================================================
  // StateMachine Component
  // ============================================================================

  class StateMachine {
  public:
    std::unordered_map<std::string, State> states;
    std::string current_state_name; // Made public for serialization
  private:
    std::queue<std::string> transition_queue_;  // Queue for deferred transitions
    bool is_first_frame_ = false;     // Track if we just entered current state
    bool has_started_ = false;        // Track if the state machine has started running
    std::vector<std::string> state_history_; // Track previous states
    Engine::Audio::ChannelHandle current_audio_channel_ = Engine::Audio::INVALID_CHANNEL; // Track currently playing audio (OLD SYSTEM)
    std::string current_fmod_instance_; // Track current FMOD instance name (NEW SYSTEM)

    Engine::ECS::Entity* owner_entity_ = nullptr;  // Reference to owning entity

  public:
    StateMachine() = default;

    // ========================================================================
    // State Management
    // ========================================================================

    /// Add a state to the state machine
    void add_state(const State& state) {
      if (states.find(state.name) != states.end()) {
        Log::warning("State '{}' already exists in state machine. Overwriting.", state.name);
      }
      states[state.name] = state;
    }

    /// Set the initial state (must be called before first update)
    void set_initial_state(const std::string& state_name) {
      if (states.find(state_name) == states.end()) {
        Log::error("Cannot set initial state '{}' - state does not exist", state_name);
        return;
      }

      current_state_name = state_name;
      is_first_frame_ = true;
      has_started_ = true;
    }

    /// Request a state change (deferred until end of current update)
    void change_state(const std::string& new_state) {
      if (states.find(new_state) == states.end()) {
        Log::error("Cannot change to state '{}' - state does not exist", new_state);
        return;
      }

      // Mark transition as pending - will execute after current update completes
      transition_queue_.push(new_state);
    }



    /// Revert to the previous state in history
    void revert_to_previous_state() {
      if (state_history_.empty()) {
        Log::warning("No previous state to revert to");
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
    void update(Engine::ECS::Entity entity) {
      // Store entity reference
      owner_entity_ = &entity;

      // Process pending transitions at the START of the frame
      // This ensures we switch states before running any update logic
      if (!transition_queue_.empty()) {
        std::string next_state = transition_queue_.front();
        transition_queue_.pop();
        execute_state_transition(next_state);

        // If we have more transitions, we could process them here, 
        // but typically one transition per frame is safer/saner.
      }

      if (current_state_name.empty()) {
        Log::warning("StateMachine has no current state set. Call set_initial_state() first.");
        return;
      }

      // Auto-initialize after deserialization
      if (!has_started_) {
        is_first_frame_ = true;
        has_started_ = true;
      }

      // Call on_enter if this is the first frame after entering this state
      if (is_first_frame_) {
        auto& current_state = states[current_state_name];
        current_state.invoke_on_enter(entity);
        is_first_frame_ = false;
      }

      // Execute current state's update
      auto& current_state = states[current_state_name];
      current_state.invoke_on_update(entity);

      // Process pending transitions at the END of the frame as well
      // This handles transitions requested during the update (e.g. animation end)
      // ensuring the state changes within the same frame.
      if (!transition_queue_.empty()) {
        std::string next_state = transition_queue_.front();
        transition_queue_.pop();
        execute_state_transition(next_state);
      }
    }

    // ========================================================================
    // Queries
    // ========================================================================

    /// Get the name of the current state
    const std::string& get_current_state() const {
      return current_state_name;
    }

    /// Check if currently in a specific state
    bool is_in_state(const std::string& state_name) const {
      return current_state_name == state_name;
    }

    /// Check if this is the first frame after entering the current state
    bool is_first_frame() const {
      return is_first_frame_;
    }

    /// Check if a state exists in this state machine
    bool has_state(const std::string& state_name) const {
      return states.find(state_name) != states.end();
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
      if (new_state == current_state_name) {
        return; // Already in this state
      }

      if (!owner_entity_) {
        Log::error("Cannot execute state transition - owner entity is null");
        return;
      }

      // Exit current state
      if (!current_state_name.empty()) {
        auto& old_state = states[current_state_name];
        old_state.invoke_on_exit(*owner_entity_);

        // Add to history (limit history size to prevent unbounded growth)
        state_history_.push_back(current_state_name);
        if (state_history_.size() > 10) { // Keep last 10 states
          state_history_.erase(state_history_.begin());
        }

        // Handle OLD audio system stop
        if (current_audio_channel_ != Engine::Audio::INVALID_CHANNEL) {
          bool should_stop = true;
          if (old_state.audio.has_value()) {
            should_stop = old_state.audio->stop_on_exit;
          }

          if (should_stop) {
            Engine::Audio::stop_channel(current_audio_channel_);
          }
          current_audio_channel_ = Engine::Audio::INVALID_CHANNEL;
        }

        // Handle NEW FMOD audio system stop
        if (!current_fmod_instance_.empty()) {
          bool should_stop = true;
          if (old_state.fmod_audio.has_value()) {
            should_stop = old_state.fmod_audio->stop_on_state_exit;
          }

          if (should_stop) {
            Engine::AudioSystem::stopEventInstance(current_fmod_instance_, true);
            Engine::AudioSystem::releaseEventInstance(current_fmod_instance_);
          }
          current_fmod_instance_.clear();
        }
      }

      // Transition to new state
      current_state_name = new_state;
      is_first_frame_ = true; // Mark that we need to call on_enter next update

      // Apply state animation if it exists
      auto& new_state_obj = states[new_state];
      if (new_state_obj.animation.has_value()) {
        apply_state_animation(new_state_obj.animation.value());
      }

      // Priority: Use FMOD audio if available, otherwise fall back to old audio system
      if (new_state_obj.fmod_audio.has_value()) {
        current_fmod_instance_ = apply_fmod_state_audio(new_state_obj.fmod_audio.value());
      }
      else if (new_state_obj.audio.has_value()) {
        current_audio_channel_ = apply_state_audio(new_state_obj.audio.value());
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

      // Apply optional collider override if specified
      if (anim.collider_data.has_value()) {
        if (auto* box = owner_entity_->get_component<BoxCollider>()) {
          box->width = anim.collider_data->width;
          box->height = anim.collider_data->height;
          box->offset = anim.collider_data->offset;
          box->aabb.half_width = box->width / 2.0f;
          box->aabb.half_height = box->height / 2.0f;
        }
      }
    }

    // Apply the audio data (OLD SYSTEM)
    Engine::Audio::ChannelHandle apply_state_audio(const StateAudio& audio) {
      if (!owner_entity_) return Engine::Audio::INVALID_CHANNEL;

      // Get or add Sound component
      Sound* sound_comp = owner_entity_->get_component<Sound>();
      if (!sound_comp) {
        Log::warning("Sound component not found for entity {}", owner_entity_->get_id());
        owner_entity_->add_component<Sound>();

        sound_comp = owner_entity_->get_component<Sound>();

      }

      // Update Sound component data
      sound_comp->sound_handle = audio.sound.sound_handle;

      // Play the sound via the component
      return sound_comp->play(audio.volume, audio.loop);
    }

    // Apply FMOD state audio (NEW SYSTEM)
    std::string apply_fmod_state_audio(const FMODStateAudio& fmod_audio) {
      if (!owner_entity_) return "";

      if (fmod_audio.event_path.empty()) {
        return "";
      }

      // Generate unique instance name using entity ID and state name
      std::string instance_name = "StateAudio_" + std::to_string(owner_entity_->get_id()) + "_" + current_state_name;

      // If looping, create persistent instance
      if (fmod_audio.loop) {
        Engine::AudioSystem::createEventInstance(instance_name, fmod_audio.event_path);
        Engine::AudioSystem::setEventInstanceVolume(instance_name, fmod_audio.volume);
        Engine::AudioSystem::startEventInstance(instance_name);
        return instance_name;
      }
      else {
        // One-shot event
        Engine::AudioSystem::playEvent(fmod_audio.event_path);
        return ""; // No instance to track for one-shot
      }
    }
  };

} // namespace Components

REFLECT_FIELDS(Components::ColliderData, width, height, offset, vertices)
REFLECT_FIELDS(Components::State, name, on_enter_callback, on_update_callback, on_exit_callback, animation, audio, fmod_audio)
REFLECT_FIELDS(Components::StateAnimation, texture, texture_atlas, animation, scale, collider_data)
REFLECT_FIELDS(Components::StateAudio, sound, volume, loop, stop_on_exit)
REGISTER_COMPONENT(Components::StateMachine, states, current_state_name)
