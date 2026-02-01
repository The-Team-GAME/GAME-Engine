//-----------------------------------------------------------------------------------
// file:    GraphicsSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of ECS graphics systems for rendering different types
//          of visual components. Includes systems for static sprites, animated
//          sprites, text rendering, distortion effects, and particle systems.
//          Each system handles specific component combinations for efficient
//          batched rendering.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Graphics/Renderer.h"
#include "../InputManager.h"
#include "../FileIO/SettingsManager.h"
#include "../FileIO/Settings.h"
#include "../GUI/TilemapPanel.h"
#include "../Logger.h"
#include "../Easing.h"
#include "../FunctionRegistry.h"
#include "../Time.h"

namespace Engine
{
  namespace Systems
  {
    class StaticSpriteRenderSystem : public Engine::ECS::System {
    public:
      StaticSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture>();
        exclude_components<Components::TextureAtlas, Components::Distortion >();
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* flip = entity.get_component<Components::Flip>();

        Math::Vec2D flip_value = flip ? flip->value : Math::Vec2D(1.0f, 1.0f);

        Engine::Graphics::draw_texture(
          texture->texture_handle,
          Math::Vec2D(transform->position.x, transform->position.y),
          Math::Vec2D(transform->scale.x, transform->scale.y),
          transform->rotation,
          *color,
          flip_value
        );
      }
    };

    class AtlasSpriteRenderSystem : public Engine::ECS::System {
    public:
      AtlasSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture, Components::TextureAtlas>();
        exclude_components<Components::Distortion >();
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();
        auto* flip = entity.get_component<Components::Flip>();

        Math::Vec2D flip_value = flip ? flip->value : Math::Vec2D(1.0f, 1.0f);

        // Ensure drawIndex is within bounds
        if (atlas->drawIndex < atlas->subTextures.size()) {
          const auto& subTexture = atlas->subTextures[atlas->drawIndex];

          Engine::Graphics::draw_subtexture(
            texture->texture_handle,
            subTexture.tex_coords,
            Math::Vec2D(transform->position.x, transform->position.y),
            Math::Vec2D(transform->scale.x, transform->scale.y),
            transform->rotation,
            *color,
            flip_value
          );
        }
      }
    };

    class TilemapRenderSystem : public Engine::ECS::System {
    public:
      TilemapRenderSystem() {
        require_components<Components::TileMap>();
      }

      void update_entity(Engine::ECS::Entity entity) override {  // Determine the color based on tilemap mode and selected layer
        auto* color_comp = entity.get_component<Components::Color>();
        if (!color_comp) {
          entity.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);
        }
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* int_map = entity.get_component<Components::TileMap>();
        auto* color_comp = entity.get_component<Components::Color>();

        // Skip if tilemap is empty
        if (!int_map || int_map->is_empty()) {
          return;
        }

        // Get the atlas entries from project settings
        const auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get();

        Math::Vec4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (color_comp)
        {
          color = *color_comp;
        }

        // Check if tilemap mode is enabled AND dimming is enabled
        if (TilemapPanel::is_tilemap_mode_enabled() && TilemapPanel::is_dim_non_selected_layers_enabled()) {
          // Get the selected layer name
          std::string selected_layer = TilemapPanel::get_selected_layer();

          // Check if this entity is on the selected layer
          bool on_selected_layer = entity.is_on_layer(selected_layer);

          if (!on_selected_layer) {
            // Darken tiles on non-selected layers
            color.x *= 0.3f;
            color.y *= 0.3f;
            color.z *= 0.3f;
          }
        }

        // Draw the tilemap using the optimized renderer function with multiple atlases
        Engine::Graphics::draw_tilemap(
          int_map->tiles,
          project_settings.loaded_tilemaps,
          int_map->tile_size,
          int_map->grid_offset,
          color
        );
      }
    };

    class TextRenderSystem : public Engine::ECS::System {
    public:
      TextRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Font, Components::Text>();
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* font = entity.get_component<Components::Font>();
        auto* text = entity.get_component<Components::Text>();

        // Use the average of scale.x and scale.y as the text scale
        float text_scale = (transform->scale.x + transform->scale.y) * 0.5f;

        Engine::Graphics::draw_text(
          font->font_handle,
          text->content,
          Math::Vec2D(transform->position.x, transform->position.y),
          text_scale,
          *color
        );
      }
    };

    class AnimationUpdateSystem : public Engine::ECS::System {
    public:
      AnimationUpdateSystem() {
        require_components<Components::Animation, Components::TextureAtlas>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* animation = entity.get_component<Components::Animation>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();

        if (animation->frames.empty()) {
          return;
        }

        // Calculate elapsed time since animation started
        float elapsed = Engine::Time::get_time() - animation->startTime;

        float time_to_use = elapsed;

        // Handle looping vs non-looping animations
        if (animation->is_looping) {
          // Loop the animation by wrapping elapsed time
          time_to_use = fmod(elapsed, animation->totalDuration);
        }
        else {
          // Clamp to the end of the animation (stay at last frame)
          if (time_to_use > animation->totalDuration) {
            time_to_use = animation->totalDuration;
          }
        }

        // Find which frame should be displayed based on elapsed time
        int current_frame = 0;
        for (int i = 0; i < animation->frames.size(); ++i) {
          if (time_to_use >= animation->frames[i].before_time) {
            current_frame = i;
          }
          else {
            break;
          }
        }

        // Update the atlas drawIndex using the frame's atlas_index
        if (current_frame >= 0 && current_frame < static_cast<int>(animation->frames.size())) {
          atlas->drawIndex = animation->frames[current_frame].atlas_index;
        }
        else {
          Log::warning("current_frame out of bounds in AnimationUpdateSystem");
          atlas->drawIndex = 0; // Fallback to first frame
        }
      }
    };

    class CameraSystem : public Engine::ECS::System {
    private:
      inline static Engine::Graphics::Camera camera{}; // this is the camera that will be sent to the renderer

    public:
      CameraSystem() {
        require_components<Components::Camera, Components::Transform2D>();
      }

      void update() override {
        Engine::ECS::for_each_entity_with<Components::Camera, Components::Transform2D>(
          [this](Engine::ECS::Entity entity) {
            auto* camera_component = entity.get_component<Components::Camera>();
            auto* transform = entity.get_component<Components::Transform2D>();

            // If this camera is marked as active, use its parameters
            if (camera_component->is_active) {
              camera.position = transform->position;
              camera.rotation = transform->rotation;
              camera.zoom = transform->scale.x; // Use x scale for zoom TODO: have x and y zoom
            }
          }
        );

        Engine::Graphics::set_camera(camera);
      }

      static void unactivate_all_cameras() {
        // Mark all cameras as inactive
        Engine::ECS::for_each_entity_with<Components::Camera>(
          [](Engine::ECS::Entity entity) {
            auto* camera_component = entity.get_component<Components::Camera>();
            camera_component->is_active = false;
          }
        );
      }

      static void reset_camera() {
        unactivate_all_cameras();

        camera.position = Math::Vec2D(0.0f, 0.0f);
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
        Engine::Graphics::set_camera(camera);
      }


      static Engine::Graphics::Camera& get_camera() {
        return camera;
      }
    };

    class CameraControlSystem : public Engine::ECS::System {
    private:
      bool is_dragging = false;
      glm::vec2 last_mouse_pos = { 0.0f, 0.0f };

    public:
      CameraControlSystem() {
        // No component requirements - this system works globally
        set_editor_system(); // Editor system - runs even when paused
      }

      void update() override {
        // Get current mouse position if in viewport
        auto mouse_pos_opt = Engine::Input::get_mouse_position_if_in_viewport();

        // Get scroll offset first (to consume it)
        glm::vec2 scroll = Engine::Input::get_scroll_offset();

        // Early exit if mouse is not in viewport
        if (!mouse_pos_opt.has_value()) {
          is_dragging = false;
          return;
        }

        glm::vec2 current_mouse_pos = mouse_pos_opt.value();

        // Check if middle mouse button is held
        bool mouse_middle_held = Engine::Input::is_held(Engine::KeyCode::MouseMiddle);

        // Start dragging with middle mouse button
        if (Engine::Input::is_pressed(Engine::KeyCode::MouseMiddle)) {
          is_dragging = true;
          last_mouse_pos = current_mouse_pos;
        }

        // Stop dragging
        if (!mouse_middle_held) {
          is_dragging = false;
        }

        auto& camera = CameraSystem::get_camera();

        // Handle zoom with scroll (no Ctrl required)
        if (scroll.y != 0.0f) {
          // Unactivate all cameras to use the free camera
          CameraSystem::unactivate_all_cameras();

          float zoom_speed = 0.1f;
          float zoom_delta = scroll.y * zoom_speed;
          camera.zoom += zoom_delta;
          // Clamp zoom to reasonable values
          camera.zoom = glm::clamp(camera.zoom, 0.1f, 10.0f);
        }

        // Perform camera drag with middle mouse button
        if (is_dragging && mouse_middle_held) {
          // Unactivate all cameras to use the free camera
          CameraSystem::unactivate_all_cameras();

          // Calculate mouse delta
          glm::vec2 delta = current_mouse_pos - last_mouse_pos;
          last_mouse_pos = current_mouse_pos;

          // Move camera in opposite direction of mouse drag for intuitive feel
          // Also account for camera zoom
          camera.position.x -= delta.x / camera.zoom;
          camera.position.y -= delta.y / camera.zoom;
        }

        // Update the camera in the graphics system
        Engine::Graphics::set_camera(camera);
      }
    };

    class DistortionSpriteRenderSystem : public Engine::ECS::System {
    public:
      DistortionSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture, Components::Distortion>();
        exclude_components<Components::TextureAtlas>();
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* distortion = entity.get_component<Components::Distortion>();
        auto* flip = entity.get_component<Components::Flip>();

        Math::Vec2D flip_value = flip ? flip->value : Math::Vec2D(1.0f, 1.0f);

        Engine::Graphics::draw_texture_distortion(
          texture->texture_handle,
          Math::Vec2D(transform->position.x, transform->position.y),
          Math::Vec2D(transform->scale.x, transform->scale.y),
          transform->rotation,
          *color,
          distortion->intensity,
          distortion->speed,
          distortion->mesh_intensity,
          flip_value
        );
      }
    };

    class DistortionAtlasSpriteRenderSystem : public Engine::ECS::System {
    public:
      DistortionAtlasSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture, Components::TextureAtlas, Components::Distortion>();
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();
        auto* distortion = entity.get_component<Components::Distortion>();
        auto* flip = entity.get_component<Components::Flip>();

        Math::Vec2D flip_value = flip ? flip->value : Math::Vec2D(1.0f, 1.0f);

        // Ensure drawIndex is within bounds
        if (atlas->drawIndex < atlas->subTextures.size()) {
          const auto& subTexture = atlas->subTextures[atlas->drawIndex];

          Engine::Graphics::draw_subtexture_distortion(
            texture->texture_handle,
            subTexture.tex_coords,
            Math::Vec2D(transform->position.x, transform->position.y),
            Math::Vec2D(transform->scale.x, transform->scale.y),
            transform->rotation,
            *color,
            distortion->intensity,
            distortion->speed,
            distortion->mesh_intensity,
            flip_value
          );
        }
      }
    };

    // ============================================================================
    // Transform Animation System
    // ============================================================================

    class TransformAnimationSystem : public Engine::ECS::System {
    private:
      using EasingFunc = f32(f32, f32, f32, f32);

      // Helper to get easing function, returns linear if not found
      static std::function<EasingFunc> get_easing(const std::string& name) {
        auto func = FunctionRegistry<EasingFunc>::get_function(name);
        if (!func) {
          return Engine::Easing::EaseLinearNone;
        }
        return func;
      }

      // Helper to interpolate between two keyframes
      static void interpolate_transform(
        Components::Transform2D* transform,
        const Components::TransformKeyframe& from,
        const Components::TransformKeyframe& to,
        float local_t,  // 0-1 between these two keyframes
        float duration_between  // duration between keyframes in seconds
      ) {
        // Get easing functions for each property
        auto pos_ease = get_easing(to.position_easing);
        auto rot_ease = get_easing(to.rotation_easing);
        auto scl_ease = get_easing(to.scale_easing);

        // Apply easing: t=local_t*duration, b=0, c=1, d=duration
        float eased_pos = pos_ease(local_t * duration_between, 0.0f, 1.0f, duration_between);
        float eased_rot = rot_ease(local_t * duration_between, 0.0f, 1.0f, duration_between);
        float eased_scl = scl_ease(local_t * duration_between, 0.0f, 1.0f, duration_between);

        // Interpolate position
        transform->position.x = from.position.x + (to.position.x - from.position.x) * eased_pos;
        transform->position.y = from.position.y + (to.position.y - from.position.y) * eased_pos;

        // Interpolate rotation
        transform->rotation = from.rotation + (to.rotation - from.rotation) * eased_rot;

        // Interpolate scale
        transform->scale.x = from.scale.x + (to.scale.x - from.scale.x) * eased_scl;
        transform->scale.y = from.scale.y + (to.scale.y - from.scale.y) * eased_scl;
      }

    public:
      TransformAnimationSystem() {
        require_components<Components::Transform2D, Components::TransformAnimation>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* anim = entity.get_component<Components::TransformAnimation>();

        // Skip if not playing or no keyframes
        if (!anim->is_playing || anim->keyframes.empty()) {
          return;
        }

        // Initialize on first run
        if (!anim->has_initialized) {
          anim->initial_position = transform->position;
          anim->initial_rotation = transform->rotation;
          anim->initial_scale = transform->scale;
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
              // Ping-pong: reverse direction
              anim->is_reversed = !anim->is_reversed;
            }
            // Reset timer
            anim->start_time = Engine::Time::get_time();
            anim->elapsed = 0.0f;
            normalized_time = 0.0f;
          }
          else {
            // Animation finished - clamp to end
            normalized_time = 1.0f;
            anim->is_playing = false;
          }
        }

        // Apply ping-pong reversal
        float effective_time = anim->is_reversed ? (1.0f - normalized_time) : normalized_time;

        // Find the two keyframes to interpolate between
        const auto& keyframes = anim->keyframes;
        size_t num_keyframes = keyframes.size();

        // Single keyframe - just set the transform
        if (num_keyframes == 1) {
          const auto& kf = keyframes[0];
          if (anim->use_relative) {
            transform->position = anim->initial_position + kf.position;
            transform->rotation = anim->initial_rotation + kf.rotation;
            transform->scale = anim->initial_scale + kf.scale;
          }
          else {
            transform->position = kf.position;
            transform->rotation = kf.rotation;
            transform->scale = kf.scale;
          }
          return;
        }

        // Find keyframe pair
        size_t from_idx = 0;
        size_t to_idx = 1;

        for (size_t i = 0; i < num_keyframes - 1; ++i) {
          if (effective_time >= keyframes[i].time && effective_time <= keyframes[i + 1].time) {
            from_idx = i;
            to_idx = i + 1;
            break;
          }
          // Handle case where we're past the last keyframe
          if (i == num_keyframes - 2 && effective_time > keyframes[i + 1].time) {
            from_idx = i;
            to_idx = i + 1;
          }
        }

        const auto& from_kf = keyframes[from_idx];
        const auto& to_kf = keyframes[to_idx];

        // Calculate local interpolation factor between these two keyframes
        float keyframe_duration = to_kf.time - from_kf.time;
        float local_t = (keyframe_duration > 0.0001f)
          ? (effective_time - from_kf.time) / keyframe_duration
          : 0.0f;
        local_t = std::clamp(local_t, 0.0f, 1.0f);

        // Create adjusted keyframes for relative mode
        Components::TransformKeyframe adjusted_from = from_kf;
        Components::TransformKeyframe adjusted_to = to_kf;

        if (anim->use_relative) {
          adjusted_from.position = anim->initial_position + from_kf.position;
          adjusted_from.rotation = anim->initial_rotation + from_kf.rotation;
          adjusted_from.scale = anim->initial_scale + from_kf.scale;

          adjusted_to.position = anim->initial_position + to_kf.position;
          adjusted_to.rotation = anim->initial_rotation + to_kf.rotation;
          adjusted_to.scale = anim->initial_scale + to_kf.scale;
        }

        // Interpolate!
        interpolate_transform(transform, adjusted_from, adjusted_to, local_t, keyframe_duration * anim->duration);
      }
    };


  }; // namespace Systems
} // namespace Engine
