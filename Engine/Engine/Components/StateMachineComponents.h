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
#include "AudioComponents.h"
#include "AnimatorComponents.h"
#include "StateMachineBlueprintRegistry.h"
#include "../Graphics/Renderer.h"
#include <cmath>

namespace Components
{
  // ============================================================================
  // State Audio Definition (FMOD Studio)
  // ============================================================================

  // StateAudio is now an alias for FMODStateAudio
  using StateAudio = FMODStateAudio;

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
    std::optional<Math::Vec2D> scale; // Scale override for this state
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
    // Optional audio data for this state
    std::optional<FMODStateAudio> audio;
    // Optional property animation clip for this state (animates component fields over time)
    std::optional<PropertyAnimationClip> property_animation;

    State() = default;

    // Construct with just a name + optional animation/audio.
    // Callbacks are left empty — sync_with_blueprint() fills them in from the blueprint.
    State(const std::string& state_name,
      std::optional<StateAnimation> state_animation = std::nullopt,
      std::optional<FMODStateAudio> state_audio = std::nullopt)
      : name(state_name)
      , animation(state_animation)
      , audio(state_audio)
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
    std::string blueprint_id;       // Links to a StateMachineBlueprintRegistry entry
    bool property_anim_playing = false; // Whether the property animation is playing (serialized)
  private:
    std::queue<std::string> transition_queue_;  // Queue for deferred transitions
    bool is_first_frame_ = false;     // Track if we just entered current state
    bool has_started_ = false;        // Track if the state machine has started running
    std::vector<std::string> state_history_; // Track previous states
    std::string current_fmod_instance_; // Track current FMOD instance name
    std::string current_fmod_event_path_; // Track the event path of the running instance (editor change detection)

    // Property animation playback state (non-serialized)
    float property_anim_elapsed_ = 0.0f;

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
      // Keep startup deferred so first update can sync callbacks from blueprint.
      // This is required because code-defined states usually only carry visual/audio
      // data while gameplay callbacks are injected by sync_with_blueprint().
      has_started_ = false;
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
        sync_with_blueprint(); // Merge code-defined callbacks onto serialized data
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

      // If the current state has looping audio:
      //   - Detect if the event path was changed in the editor and restart with the new event
      //   - Keep 3D position in sync every frame
      //   - Restart if the instance stopped (loop support)
      if (current_state.audio.has_value() && current_state.audio->loop) {
        const std::string& desired_path = current_state.audio->event_path;
        if (!desired_path.empty()) {
          // Editor changed the path mid-state — stop/release old, apply new
          if (current_fmod_event_path_ != desired_path) {
            if (!current_fmod_instance_.empty()) {
              Engine::AudioSystem::stopEventInstance(current_fmod_instance_, true);
              Engine::AudioSystem::releaseEventInstance(current_fmod_instance_);
              current_fmod_instance_.clear();
            }
            current_fmod_instance_ = apply_fmod_state_audio(current_state.audio.value());
          }
          else if (!current_fmod_instance_.empty()) {
            if (auto* t = entity.get_component<Transform2D>()) {
              Engine::AudioSystem::setEventInstancePosition(current_fmod_instance_, t->position);
            }
            const float camera_mult = is_owner_entity_visible_for_audio() ? 1.0f : 0.1f;
            Engine::AudioSystem::setEventInstanceVolume(current_fmod_instance_, current_state.audio->volume * camera_mult);
            if (!Engine::AudioSystem::isEventInstancePlaying(current_fmod_instance_)) {
              Engine::AudioSystem::startEventInstance(current_fmod_instance_);
            }
          }
        }
      }

      // Evaluate property animation for current state
      evaluate_property_animation(entity, Engine::Time::get_fixed_dt());

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

    // ========================================================================
    // Property Animation Playback Control
    // ========================================================================

    /// Check if the property animation is currently playing
    bool is_property_anim_playing() const { return property_anim_playing; }

    /// Get the current elapsed time of the property animation
    float get_property_anim_elapsed() const { return property_anim_elapsed_; }

    /// Set the elapsed time of the property animation (for scrubbing / external control)
    void set_property_anim_elapsed(float t) { property_anim_elapsed_ = t; }

    /// Start or resume property animation playback
    void play_property_anim() { property_anim_playing = true; }

    /// Pause property animation playback (keeps elapsed time)
    void pause_property_anim() { property_anim_playing = false; }

    /// Stop property animation playback and reset elapsed time to 0
    void stop_property_anim() { property_anim_playing = false; property_anim_elapsed_ = 0.0f; }

    // ========================================================================
    // Blueprint Synchronization
    // ========================================================================

    /// Merge callbacks from the code-defined blueprint onto the serialized
    /// state data.  Animation / audio / collider data in the prefab is
    /// preserved.  States that exist in the blueprint but not in the prefab
    /// are added (with no animation).  States that exist in the prefab but
    /// not in the blueprint are removed.
    void sync_with_blueprint() {
      if (blueprint_id.empty()) return;

      const auto* bp = Engine::StateMachineBlueprintRegistry::instance()
        .get_blueprint(blueprint_id);
      if (!bp) {
        Log::warning("StateMachine: blueprint '{}' not found in registry", blueprint_id);
        return;
      }

      // Build a set of blueprint state names for quick lookup
      std::unordered_map<std::string, const Engine::StateBlueprint*> bp_map;
      for (const auto& sb : bp->states) {
        bp_map[sb.state_name] = &sb;
      }

      // 1. Update existing states & add new ones
      for (const auto& sb : bp->states) {
        auto it = states.find(sb.state_name);
        if (it != states.end()) {
          // State already exists — overwrite callbacks only
          it->second.on_enter_callback = sb.on_enter;
          it->second.on_update_callback = sb.on_update;
          it->second.on_exit_callback = sb.on_exit;
        }
        else {
          // New state from blueprint — add with callbacks but no animation/audio
          State new_state;
          new_state.name = sb.state_name;
          new_state.on_enter_callback = sb.on_enter;
          new_state.on_update_callback = sb.on_update;
          new_state.on_exit_callback = sb.on_exit;
          states[sb.state_name] = new_state;
          Log::info("StateMachine: added new state '{}' from blueprint '{}'", sb.state_name, blueprint_id);
        }
      }

      // 2. Remove states that are no longer in the blueprint
      for (auto it = states.begin(); it != states.end(); ) {
        if (bp_map.find(it->first) == bp_map.end()) {
          Log::info("StateMachine: removed stale state '{}' (not in blueprint '{}')", it->first, blueprint_id);
          it = states.erase(it);
        }
        else {
          ++it;
        }
      }

      // 3. Fix current_state_name if it was removed
      if (!current_state_name.empty() && states.find(current_state_name) == states.end()) {
        if (!bp->initial_state.empty() && states.find(bp->initial_state) != states.end()) {
          current_state_name = bp->initial_state;
        }
        else if (!states.empty()) {
          current_state_name = states.begin()->first;
        }
        Log::warning("StateMachine: current state was removed, reset to '{}'", current_state_name);
      }
    }

    /// Force an immediate state switch and apply its animation/audio to the entity.
    /// Intended for use in the editor where the normal update loop may not be running.
    void set_state_immediate(const std::string& state_name, Engine::ECS::Entity entity) {
      if (states.find(state_name) == states.end()) {
        Log::error("Cannot set state '{}' - state does not exist", state_name);
        return;
      }
      owner_entity_ = &entity;
      current_state_name = state_name;
      is_first_frame_ = true;

      // Apply visual/audio data immediately
      auto& state = states[state_name];
      if (state.animation.has_value()) {
        apply_state_animation(state.animation.value());
      }
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

        // Handle FMOD audio system stop
        if (!current_fmod_instance_.empty()) {
          Engine::AudioSystem::stopEventInstance(current_fmod_instance_, true);
          Engine::AudioSystem::releaseEventInstance(current_fmod_instance_);
          current_fmod_instance_.clear();
          current_fmod_event_path_.clear();
        }
      }

      // Transition to new state
      current_state_name = new_state;
      is_first_frame_ = true; // Mark that we need to call on_enter next update

      // Reset property animation playback for new state
      property_anim_elapsed_ = 0.0f;
      auto& new_state_check = states[new_state];
      property_anim_playing = new_state_check.property_animation.has_value();

      // Apply state animation if it exists
      auto& new_state_obj = states[new_state];
      if (new_state_obj.animation.has_value()) {
        apply_state_animation(new_state_obj.animation.value());
      }

      // Apply FMOD audio if available
      if (new_state_obj.audio.has_value()) {
        current_fmod_instance_ = apply_fmod_state_audio(new_state_obj.audio.value());
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

      // Apply scale override if specified
      if (anim.scale.has_value()) {
        if (auto* transform = owner_entity_->get_component<Transform2D>()) {
          transform->scale = anim.scale.value();
        }
      }

      // Apply optional collider override if specified
      if (anim.collider_data.has_value()) {
        if (auto* box = owner_entity_->get_component<BoxCollider>()) {
          box->type = anim.collider_data->type;
          box->width = anim.collider_data->width;
          box->height = anim.collider_data->height;
          box->offset = anim.collider_data->offset;
          box->aabb.half_width = box->width / 2.0f;
          box->aabb.half_height = box->height / 2.0f;
          
          if (box->type == Components::ColliderType::POLYGON) {
            box->polygon.vertices = anim.collider_data->vertices;
          }
        }
      }
    }

  public:
    // ========================================================================
    // Property Animation Evaluation
    // ========================================================================

    /// Navigate a GenericValue object by a dot-separated field path and set a double value
    static bool set_generic_value_at_path(ReflectionLibrary::GenericValue& root, const std::string& path, double value) {
      // Split path by '.'
      std::vector<std::string> segments;
      std::string segment;
      for (char c : path) {
        if (c == '.') {
          if (!segment.empty()) { segments.push_back(segment); segment.clear(); }
        }
        else {
          segment += c;
        }
      }
      if (!segment.empty()) segments.push_back(segment);
      if (segments.empty()) return false;

      // Navigate to parent of the final field
      ReflectionLibrary::GenericValue* current = &root;
      for (size_t i = 0; i < segments.size() - 1; ++i) {
        if (!current->isObject()) return false;
        auto& obj = current->getObject();
        auto it = obj.find(segments[i]);
        if (it == obj.end()) return false;
        current = &it->second;
      }

      // Set the final field
      const std::string& final_key = segments.back();
      if (current->isObject()) {
        auto& obj = current->getObject();
        auto it = obj.find(final_key);
        if (it != obj.end()) {
          if (it->second.isDouble()) {
            it->second = ReflectionLibrary::GenericValue(value);
            return true;
          }
          else if (it->second.isInt()) {
            it->second = ReflectionLibrary::GenericValue(static_cast<int64_t>(std::round(value)));
            return true;
          }
        }
      }
      return false;
    }

  private:
    /// Evaluate property animation for the current state and apply values
    void evaluate_property_animation(Engine::ECS::Entity entity, float dt) {
      if (!property_anim_playing) return;

      auto it = states.find(current_state_name);
      if (it == states.end() || !it->second.property_animation.has_value()) {
        property_anim_playing = false;
        return;
      }

      const auto& clip = it->second.property_animation.value();
      if (clip.tracks.empty() || clip.duration <= 0.0f) return;

      // Advance elapsed time
      property_anim_elapsed_ += dt;

      if (property_anim_elapsed_ >= clip.duration) {
        if (clip.is_looping) {
          property_anim_elapsed_ = std::fmod(property_anim_elapsed_, clip.duration);
        }
        else {
          property_anim_elapsed_ = clip.duration;
          property_anim_playing = false; // Animation finished
        }
      }

      // Group tracks by target component to batch reads/writes
      std::unordered_map<std::string, std::vector<const AnimationTrack*>> grouped_tracks;
      for (const auto& track : clip.tracks) {
        grouped_tracks[track.target_component].push_back(&track);
      }

      // Apply each component group
      entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        auto group_it = grouped_tracks.find(type_name);
        if (group_it == grouped_tracks.end()) return;
        if (!array->is_reflectable()) return;

        // Read component as GenericValue
        auto component_data = array->to_generic_value(entity.get_id());
        bool any_modified = false;

        // Apply each track in this group
        for (const auto* track : group_it->second) {
          double interpolated = track->evaluate(property_anim_elapsed_);
          if (set_generic_value_at_path(component_data, track->target_field_path, interpolated)) {
            any_modified = true;
          }
        }

        // Write back if any tracks modified the component
        if (any_modified) {
          array->set_component_from_generic_value(entity.get_id(), component_data);
        }
        });
    }

    // Apply FMOD state audio (NEW SYSTEM)
    std::string apply_fmod_state_audio(const FMODStateAudio& fmod_audio) {
      if (!owner_entity_) return "";

      if (fmod_audio.event_path.empty()) {
        return "";
      }

      // Generate unique instance name using entity ID and state name
      std::string instance_name = "StateAudio_" + std::to_string(owner_entity_->get_id()) + "_" + current_state_name;

      // Get entity world position for spatial audio
      Math::Vec2D entity_pos = { 0.0f, 0.0f };
      if (auto* t = owner_entity_->get_component<Transform2D>()) {
        entity_pos = t->position;
      }

      // If looping, create persistent instance
      if (fmod_audio.loop) {
        const float camera_mult = is_owner_entity_visible_for_audio() ? 1.0f : 0.1f;

        // Always release any pre-existing instance with this name so editor path changes take effect
        Engine::AudioSystem::stopEventInstance(instance_name, true);
        Engine::AudioSystem::releaseEventInstance(instance_name);

        Engine::AudioSystem::createEventInstance(instance_name, fmod_audio.event_path);
        Engine::AudioSystem::setEventInstancePosition(instance_name, entity_pos);
        Engine::AudioSystem::setEventInstanceVolume(instance_name, fmod_audio.volume * camera_mult);
        Engine::AudioSystem::startEventInstance(instance_name);
        current_fmod_event_path_ = fmod_audio.event_path;
        return instance_name;
      }
      else {
        // One-shot event at entity's world position
        const float camera_mult = is_owner_entity_visible_for_audio() ? 1.0f : 0.1f;
        Engine::AudioSystem::playEventAtPosition(fmod_audio.event_path, entity_pos, 500.0f, 2000.0f, 50.0f, fmod_audio.volume * camera_mult);
        return ""; // No instance to track for one-shot
      }
    }

    bool is_owner_entity_visible_for_audio() const {
      if (!owner_entity_) return true;

      auto* t = owner_entity_->get_component<Transform2D>();
      if (!t) return true;

      Math::Vec2D approx_size = {
        std::max(1.0f, std::abs(t->scale.x)),
        std::max(1.0f, std::abs(t->scale.y))
      };
      return Engine::Graphics::is_visible(t->position, approx_size);
    }
  };

} // namespace Components

REFLECT_FIELDS(Components::ColliderData, type, width, height, offset, vertices)
REFLECT_FIELDS(Components::State, name, on_enter_callback, on_update_callback, on_exit_callback, animation, audio, property_animation)
REFLECT_FIELDS(Components::StateAnimation, texture, texture_atlas, animation, scale, collider_data)
REGISTER_COMPONENT(Components::StateMachine, states, current_state_name, blueprint_id, property_anim_playing)
