//-----------------------------------------------------------------------------------
// file:    AnimatorSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   System that processes Animator components, resolving field paths to
//          actual memory pointers and interpolating values based on keyframes.
//          Uses reflection to access any numeric field in any component.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/AnimatorComponents.h"
#include "../Easing.h"
#include "../FunctionRegistry.h"
#include "../Time.h"
#include "../Logger.h"
#include <unordered_map>
#include <functional>

namespace Engine
{
  namespace Systems
  {
    class AnimatorSystem : public Engine::ECS::System {
    private:
      using EasingFunc = f32(f32, f32, f32, f32);

      // Runtime cache for resolved field pointers
      // Key: (EntityID << 32) | track_index
      struct ResolvedTrack {
        float* target_ptr = nullptr;
        bool is_valid = false;
      };
      mutable std::unordered_map<u64, ResolvedTrack> resolved_cache;

      // Helper to get easing function
      static std::function<EasingFunc> get_easing(const std::string& name) {
        auto func = FunctionRegistry<EasingFunc>::get_function(name);
        if (!func) {
          return Engine::Easing::EaseLinearNone;
        }
        return func;
      }

      // Helper to make cache key
      static u64 make_cache_key(Engine::ECS::EntityID entity_id, size_t track_index) {
        return (static_cast<u64>(entity_id) << 32) | static_cast<u64>(track_index);
      }

      // Resolve a field path to a float pointer
      // Returns nullptr if the path cannot be resolved
      float* resolve_field_path(
        Engine::ECS::Entity entity,
        const std::string& component_type,
        const std::string& field_path
      ) const {
        auto& containers = Engine::ECS::Internal::get_containers();

        // Find the component array by type name
        for (auto& [type_id, array] : containers.component_arrays) {
          if (array->get_type_name() != component_type) {
            continue;
          }

          // Check if entity has this component
          if (!array->is_reflectable()) {
            Log::warning("AnimatorSystem: Component '{}' is not reflectable", component_type);
            return nullptr;
          }

          // Get the component as GenericValue
          auto generic = array->to_generic_value(entity.get_id());
          if (!generic.isObject()) {
            return nullptr;
          }

          // Parse the field path (supports nested paths like "position.x")
          std::vector<std::string> path_parts;
          size_t start = 0;
          size_t pos = 0;
          while ((pos = field_path.find('.', start)) != std::string::npos) {
            path_parts.push_back(field_path.substr(start, pos - start));
            start = pos + 1;
          }
          path_parts.push_back(field_path.substr(start));

          // We need to get the actual memory address of the field
          // This requires going through the component array's internal storage
          // For now, we'll use a different approach: store the value and path,
          // and use set_component_from_generic_value to write back

          // Unfortunately, getting a raw pointer to a field in a component
          // requires knowledge of the component's memory layout.
          // The safest approach is to use the GenericValue system for reads/writes.

          return nullptr; // We'll use a different approach
        }

        return nullptr;
      }

      // Interpolate a track's value and apply it
      void interpolate_and_apply_track(
        Engine::ECS::Entity entity,
        const Components::AnimationTrack& track,
        float normalized_time
      ) const {
        if (track.keyframes.empty()) return;

        auto& containers = Engine::ECS::Internal::get_containers();

        // Find the component array
        Engine::ECS::IComponentArray* target_array = nullptr;
        for (auto& [type_id, array] : containers.component_arrays) {
          if (array->get_type_name() == track.component_type) {
            target_array = array.get();
            break;
          }
        }

        if (!target_array || !target_array->is_reflectable()) {
          return;
        }

        // Get current component data
        auto generic = target_array->to_generic_value(entity.get_id());
        if (!generic.isObject()) {
          return;
        }

        // Find the two keyframes to interpolate between
        const auto& keyframes = track.keyframes;
        size_t num_keyframes = keyframes.size();

        // Single keyframe - just set the value
        if (num_keyframes == 1) {
          set_field_value(generic, track.field_path, keyframes[0].value);
          target_array->set_component_from_generic_value(entity.get_id(), generic);
          return;
        }

        // Find keyframe pair
        size_t from_idx = 0;
        size_t to_idx = 1;

        for (size_t i = 0; i < num_keyframes - 1; ++i) {
          if (normalized_time >= keyframes[i].time && normalized_time <= keyframes[i + 1].time) {
            from_idx = i;
            to_idx = i + 1;
            break;
          }
          // Handle case where we're past the last keyframe
          if (i == num_keyframes - 2 && normalized_time > keyframes[i + 1].time) {
            from_idx = i;
            to_idx = i + 1;
          }
        }

        const auto& from_kf = keyframes[from_idx];
        const auto& to_kf = keyframes[to_idx];

        // Calculate local interpolation factor
        float keyframe_duration = to_kf.time - from_kf.time;
        float local_t = (keyframe_duration > 0.0001f)
          ? (normalized_time - from_kf.time) / keyframe_duration
          : 0.0f;
        local_t = std::clamp(local_t, 0.0f, 1.0f);

        // Apply easing
        auto easing_func = get_easing(to_kf.easing);
        float duration_seconds = keyframe_duration; // Use normalized as if it were seconds for easing
        float eased_t = easing_func(local_t * duration_seconds, 0.0f, 1.0f, duration_seconds);

        // Interpolate value
        float interpolated_value = from_kf.value + (to_kf.value - from_kf.value) * eased_t;

        // Set the field value
        set_field_value(generic, track.field_path, interpolated_value);

        // Write back to component
        target_array->set_component_from_generic_value(entity.get_id(), generic);
      }

      // Helper to set a nested field value in a GenericValue
      static void set_field_value(
        ReflectionLibrary::GenericValue& obj,
        const std::string& field_path,
        float value
      ) {
        // Parse the path
        std::vector<std::string> parts;
        size_t start = 0;
        size_t pos = 0;
        while ((pos = field_path.find('.', start)) != std::string::npos) {
          parts.push_back(field_path.substr(start, pos - start));
          start = pos + 1;
        }
        parts.push_back(field_path.substr(start));

        // Navigate and set the value using the proxy's assignment operator
        if (parts.size() == 1) {
          // Simple field (e.g., "r", "rotation")
          if (obj.isObject() && obj.has(parts[0])) {
            ReflectionLibrary::GenericValue current_val = obj[parts[0]].get();
            if (current_val.isDouble()) {
              obj[parts[0]] = static_cast<double>(value);
            }
            else if (current_val.isInt()) {
              obj[parts[0]] = static_cast<int64_t>(value);
            }
            else {
              obj[parts[0]] = static_cast<double>(value);
            }
          }
        }
        else if (parts.size() == 2) {
          // Nested field (e.g., "position.x")
          if (obj.isObject() && obj.has(parts[0])) {
            ReflectionLibrary::GenericValue nested = obj[parts[0]].get();
            if (nested.isObject() && nested.has(parts[1])) {
              ReflectionLibrary::GenericValue current_val = nested[parts[1]].get();
              if (current_val.isDouble()) {
                nested[parts[1]] = static_cast<double>(value);
              }
              else if (current_val.isInt()) {
                nested[parts[1]] = static_cast<int64_t>(value);
              }
              else {
                nested[parts[1]] = static_cast<double>(value);
              }
              obj[parts[0]] = nested;
            }
          }
        }
        else {
          Log::warning("AnimatorSystem: Deep nested paths (>2 levels) not yet supported: '{}'", field_path);
        }
      }

    public:
      AnimatorSystem() {
        require_components<Components::Animator>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* anim = entity.get_component<Components::Animator>();

        // Handle force_update (editor scrubbing/keyframe editing)
        if (anim->force_update) {
          anim->force_update = false;
          
          if (anim->tracks.empty()) return;
          
          // Apply animation at current elapsed time without recalculating
          float normalized_time = anim->duration > 0.0f ? (anim->elapsed / anim->duration) : 0.0f;
          normalized_time = std::clamp(normalized_time, 0.0f, 1.0f);
          float effective_time = anim->is_reversed ? (1.0f - normalized_time) : normalized_time;
          
          for (const auto& track : anim->tracks) {
            interpolate_and_apply_track(entity, track, effective_time);
          }
          return;
        }

        if (!anim->is_playing || anim->tracks.empty()) {
          return;
        }

        // Initialize on first run
        if (!anim->has_initialized) {
          anim->has_initialized = true;
          anim->start_time = Engine::Time::get_time();
          anim->elapsed = 0.0f;
        }

        // Update elapsed time
        anim->elapsed = Engine::Time::get_time() - anim->start_time;

        // Calculate normalized time (0-1)
        float normalized_time = anim->elapsed / anim->duration;

        // Handle looping
        if (normalized_time >= 1.0f) {
          if (anim->is_looping) {
            if (anim->ping_pong) {
              anim->is_reversed = !anim->is_reversed;
            }
            anim->start_time = Engine::Time::get_time();
            anim->elapsed = 0.0f;
            normalized_time = 0.0f;
          }
          else {
            normalized_time = 1.0f;
            anim->is_playing = false;
          }
        }

        // Apply ping-pong reversal
        float effective_time = anim->is_reversed ? (1.0f - normalized_time) : normalized_time;

        // Process each track
        for (const auto& track : anim->tracks) {
          interpolate_and_apply_track(entity, track, effective_time);
        }
      }

      // Clear the cache when entities are destroyed
      void on_entity_removed(const Engine::ECS::Entity entity) override {
        // Remove all cached tracks for this entity
        auto entity_id = entity.get_id();
        for (auto it = resolved_cache.begin(); it != resolved_cache.end();) {
          u64 cached_entity_id = it->first >> 32;
          if (cached_entity_id == entity_id) {
            it = resolved_cache.erase(it);
          }
          else {
            ++it;
          }
        }
      }
    };

  } // namespace Systems
} // namespace Engine
