//-----------------------------------------------------------------------------------
// file:    GraphicsSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
//          Ryan Ang An Kiat (20%)
//          Secondary author
//
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
#include "../Random.h"
#include "../Audio/AudioSystem.h"

namespace Engine
{
  namespace Systems
  {
    // Engine-exported bridge to ensure shake requests from external modules
    // (e.g., GameLogic DLL) hit the active CameraSystem instance.
    ENGINE_API void add_camera_shake(f32 trauma);

    // Helper: compute parallax draw position (center-anchored).
    // When the camera centers on the entity, it appears at its Transform2D position.
    // As the camera moves away the entity shifts at scroll_factor speed.
    inline Math::Vec2D compute_parallax_position(const Math::Vec2D& entity_pos, const Math::Vec2D& scroll_factor) {
      auto cam = Engine::Graphics::get_camera();
      auto vres = Engine::Graphics::get_virtual_resolution();
      Math::Vec2D cam_center = cam.position + vres * 0.5f / cam.zoom;
      return Math::Vec2D(
        entity_pos.x + (cam_center.x - entity_pos.x) * (1.0f - scroll_factor.x),
        entity_pos.y + (cam_center.y - entity_pos.y) * (1.0f - scroll_factor.y)
      );
    }

    // Helper: compute the final screen-space position for a UI element.
    // final_pos = vres * anchor + Transform2D.position
    // When anchor=(0,0) this is identical to the old behaviour (raw transform position).
    // When anchor is set, Transform2D.position acts as a pixel offset from the anchor,
    // so gizmo drag works correctly out of the box.
    inline Math::Vec2D compute_ui_position(const Components::UI& ui, const Components::Transform2D& transform) {
      auto vres = Engine::Graphics::get_virtual_resolution();
      return Math::Vec2D(
        vres.x * ui.anchor.x + transform.position.x,
        vres.y * ui.anchor.y + transform.position.y
      );
    }

    // Helper: draw a texture tiled across the visible area at a given parallax draw position.
    template<typename DrawFn>
    inline void draw_parallax_tiled(const Math::Vec2D& draw_pos, const Math::Vec2D& entity_scale,
      const Components::Parallax& parallax, DrawFn draw_fn)
    {
      Math::Vec2D ts = parallax.tile_size;
      if (ts.x <= 0.0f) ts.x = entity_scale.x;
      if (ts.y <= 0.0f) ts.y = entity_scale.y;

      auto bounds = Engine::Graphics::get_camera_bounds();

      f32 wrapped_x = std::fmod(draw_pos.x - bounds.min_x + ts.x, ts.x);
      if (wrapped_x < 0.0f) wrapped_x += ts.x;
      f32 start_x = bounds.min_x - ts.x + wrapped_x;

      f32 wrapped_y = std::fmod(draw_pos.y - bounds.min_y + ts.y, ts.y);
      if (wrapped_y < 0.0f) wrapped_y += ts.y;
      f32 start_y = bounds.min_y - ts.y + wrapped_y;

      for (f32 ty = start_y; ty < bounds.max_y + ts.y; ty += ts.y) {
        for (f32 tx = start_x; tx < bounds.max_x + ts.x; tx += ts.x) {
          draw_fn(Math::Vec2D(tx, ty), ts);
        }
      }
    }

    class StaticSpriteRenderSystem : public Engine::ECS::System {
    public:
      StaticSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture>();
        exclude_components<Components::TextureAtlas, Components::Distortion>();
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* flip = entity.get_component<Components::Flip>();
        auto* ui = entity.get_component<Components::UI>();
        auto* parallax = entity.get_component<Components::Parallax>();

        Math::Vec2D flip_value = flip ? flip->value : Math::Vec2D(1.0f, 1.0f);

        if (ui) {
          // Render in screen space (not affected by camera)
          Math::Vec2D pos = compute_ui_position(*ui, *transform);
          Engine::Graphics::draw_texture_ui(
            texture->texture_handle,
            pos,
            Math::Vec2D(transform->scale.x, transform->scale.y),
            transform->rotation,
            *color,
            flip_value
          );
        }
        else if (parallax) {
          // Parallax background rendering
          Math::Vec2D draw_pos = compute_parallax_position(transform->position, parallax->scroll_factor);

          if (parallax->tiling) {
            draw_parallax_tiled(draw_pos, transform->scale, *parallax,
              [&](const Math::Vec2D& pos, const Math::Vec2D& size) {
                Engine::Graphics::draw_texture(texture->texture_handle, pos, size, transform->rotation, *color, flip_value);
              });
          }
          else {
            Engine::Graphics::draw_texture(
              texture->texture_handle, draw_pos,
              Math::Vec2D(transform->scale.x, transform->scale.y),
              transform->rotation, *color, flip_value
            );
          }
        }
        else {
          // Render in world space
          Engine::Graphics::draw_texture(
            texture->texture_handle,
            Math::Vec2D(transform->position.x, transform->position.y),
            Math::Vec2D(transform->scale.x, transform->scale.y),
            transform->rotation,
            *color,
            flip_value
          );
        }
      }
    };

    class AtlasSpriteRenderSystem : public Engine::ECS::System {
    public:
      AtlasSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture, Components::TextureAtlas>();
        exclude_components<Components::Distortion>();
      }

      void draw_entity(const Engine::ECS::Entity entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();
        auto* flip = entity.get_component<Components::Flip>();
        auto* ui = entity.get_component<Components::UI>();
        auto* parallax = entity.get_component<Components::Parallax>();

        Math::Vec2D flip_value = flip ? flip->value : Math::Vec2D(1.0f, 1.0f);

        // Ensure drawIndex is within bounds
        if (atlas->drawIndex < atlas->subTextures.size()) {
          const auto& subTexture = atlas->subTextures[atlas->drawIndex];

          if (ui) {
            // Render in screen space (not affected by camera)
            Math::Vec2D pos = compute_ui_position(*ui, *transform);
            Engine::Graphics::draw_subtexture_ui(
              texture->texture_handle,
              subTexture.tex_coords,
              pos,
              Math::Vec2D(transform->scale.x, transform->scale.y),
              transform->rotation,
              *color,
              flip_value
            );
          }
          else if (parallax) {
            // Parallax background rendering with atlas subtexture
            Math::Vec2D draw_pos = compute_parallax_position(transform->position, parallax->scroll_factor);

            if (parallax->tiling) {
              draw_parallax_tiled(draw_pos, transform->scale, *parallax,
                [&](const Math::Vec2D& pos, const Math::Vec2D& size) {
                  Engine::Graphics::draw_subtexture(texture->texture_handle, subTexture.tex_coords, pos, size, transform->rotation, *color, flip_value);
                });
            }
            else {
              Engine::Graphics::draw_subtexture(
                texture->texture_handle, subTexture.tex_coords, draw_pos,
                Math::Vec2D(transform->scale.x, transform->scale.y),
                transform->rotation, *color, flip_value
              );
            }
          }
          else {
            // Render in world space
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

        // Apply parallax offset to grid_offset if the entity has a Parallax component
        Math::Vec2D draw_offset = int_map->grid_offset;
        auto* parallax = entity.get_component<Components::Parallax>();
        if (parallax) {
          // compute_parallax_position shifts the anchor based on camera distance
          // We treat grid_offset as the anchor position for the tilemap
          draw_offset = compute_parallax_position(int_map->grid_offset, parallax->scroll_factor);
        }

        // Draw the tilemap using the optimized renderer function with multiple atlases
        Engine::Graphics::draw_tilemap(
          int_map->tiles,
          project_settings.loaded_tilemaps,
          int_map->tile_size,
          draw_offset,
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

        // Transform2D scale defines the text box bounds in world units
        // Text starts at top-left corner of the bounding box
        f32 box_width = transform->scale.x;
        f32 text_x = transform->position.x - box_width * 0.5f;
        f32 text_y = transform->position.y - transform->scale.y * 0.5f;

        // Always use wrapped text rendering with bounding box width as max_width
        // box_width is in world units, font_scale scales text relative to world units
        // So max_width should be box_width (both in world units)
        Engine::Graphics::draw_text_wrapped(
          font->font_handle,
          text->content,
          Math::Vec2D(text_x, text_y),
          text->font_scale,
          *color,
          box_width,
          text->line_spacing,
          static_cast<int>(text->alignment)
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
      inline static f32 shake_trauma = 0.0f;                // 0..1 intensity accumulator
      inline static f32 shake_max_offset = 150.0f;          // Max pixels to shake
      inline static f32 shake_trauma_decay = 0.8f;          // Trauma decay per second
      inline static f32 shake_sample_timer = 0.0f;          // timer for high-frequency sample changes
      inline static Math::Vec2D shake_sample = { 0.0f, 0.0f }; // current sampled direction

      inline static bool has_active_camera_entity() {
        bool has_active_camera = false;
        Engine::ECS::for_each_entity_with<Components::Camera>(
          [&has_active_camera](Engine::ECS::Entity entity) {
            auto* cam = entity.get_component<Components::Camera>();
            if (cam && cam->is_active) {
              has_active_camera = true;
            }
          }
        );
        return has_active_camera;
      }

      inline f32 Lerp(f32 a, f32 b, f32 t)
      {
        return a + (b - a) * t;
      }

      inline Vec2D Lerp(const Vec2D& a, const Vec2D& b, f32 t)
      {
        return a + (b - a) * t;
      }

    public:
      CameraSystem() {
        require_components<Components::Camera, Components::Transform2D>();
      }

      void update() override {
        const f32 dt = Time::get_fixed_dt();
        bool has_active_camera = false;

        Engine::ECS::for_each_entity_with<Components::Camera, Components::Transform2D>(
          [this, dt, &has_active_camera](Engine::ECS::Entity entity) {
            auto* cam = entity.get_component<Components::Camera>();
            auto* trans = entity.get_component<Components::Transform2D>();

            if (!cam || !cam->is_active) return;
            has_active_camera = true;

            // 1. Safe Parent & Flip Retrieval
            f32 look_dir = 0.0f;
            if (auto* parent_comp = entity.get_component<Components::Parent>()) {
              auto parent_entity = Engine::ECS::get_entity(parent_comp->parent_id);
              if (parent_entity.is_valid()) {
                if (auto* p_flip = parent_entity.get_component<Components::Flip>()) {
                  // Use the raw flip value (-1 or 1)
                  look_dir = p_flip->value.x;
                }
              }
            }

            // 2. Look-ahead Calculation (smooth camera leading in movement direction)
            Vec2D desired_look_ahead = { look_dir * cam->look_ahead_distance, 0.0f };

            // Smoothly interpolate look-ahead offset
            f32 t_look_ahead = 1.0f - std::exp(-cam->look_ahead_smooth * dt);
            cam->look_ahead_offset = Lerp(cam->look_ahead_offset, desired_look_ahead, t_look_ahead);

            // 3. Calculate Target Position (apply static offset + look-ahead)
            Vec2D target_position = trans->position + cam->target_offset + cam->look_ahead_offset;

            // 4. Detect Direction Change
            Vec2D current_velocity = target_position - camera.position;
            f32 direction_consistency = 1.0f;

            // Calculate how aligned current movement is with previous movement
            f32 prev_mag = std::sqrt(cam->prev_velocity.x * cam->prev_velocity.x + cam->prev_velocity.y * cam->prev_velocity.y);
            f32 curr_mag = std::sqrt(current_velocity.x * current_velocity.x + current_velocity.y * current_velocity.y);

            if (prev_mag > 0.01f && curr_mag > 0.01f) {
              // Dot product normalized = cosine of angle between vectors
              f32 dot = (cam->prev_velocity.x * current_velocity.x + cam->prev_velocity.y * current_velocity.y);
              direction_consistency = dot / (prev_mag * curr_mag);
              direction_consistency = std::max(0.0f, direction_consistency); // Clamp to 0-1
            }

            // Store current velocity for next frame
            cam->prev_velocity = current_velocity;

            // 5. Apply Dead Zone (optional - prevents jittery movement)
            if (cam->dead_zone_radius > 0.0f) {
              Vec2D to_target = target_position - camera.position;
              f32 distance = std::sqrt(to_target.x * to_target.x + to_target.y * to_target.y);

              if (distance < cam->dead_zone_radius) {
                // Within dead zone - don't move camera
                target_position = camera.position;
              }
              else {
                // Outside dead zone - adjust target to edge of dead zone
                f32 scale = (distance - cam->dead_zone_radius) / distance;
                target_position = camera.position + (to_target * scale);
              }
            }

            // 6. Apply Smooth Camera Movement
            if (cam->is_smooth) {
              // Blend smoothing factor based on direction consistency
              // Same direction (1.0) = use fast smooth_factor
              // Direction change (0.0) = use slow smooth_factor_direction_change
              f32 blended_smooth = Lerp(cam->smooth_factor_direction_change, cam->smooth_factor, direction_consistency);

              // Exponential smoothing for natural, framerate-independent movement
              f32 t_pos = 1.0f - std::exp(-blended_smooth * dt);
              f32 t_rot = 1.0f - std::exp(-cam->rotation_smooth * dt);
              f32 t_zoom = 1.0f - std::exp(-cam->zoom_smooth * dt);

              camera.position = Lerp(camera.position, target_position, t_pos);
              camera.rotation = Lerp(camera.rotation, trans->rotation, t_rot);
              camera.zoom = Lerp(camera.zoom, trans->scale.x, t_zoom);
            }
            else {
              // Instant camera (no smoothing)
              camera.position = target_position;
              camera.rotation = trans->rotation;
              camera.zoom = trans->scale.x;
            }
          });

        // Keep FMOD listener in sync with the active camera center.
        // Pull the listener forward along Z by half the visible width so
        // FMOD computes gradual stereo panning instead of snapping to 90°.
        auto vres = Engine::Graphics::get_virtual_resolution();
        f32 half_width = vres.x * 0.5f / camera.zoom;
        Engine::AudioSystem::setListenerPosition(
          camera.position + vres * 0.5f / camera.zoom, -half_width);

        Engine::Graphics::Camera render_camera = camera;

        if (shake_trauma > 0.0f) {
          shake_trauma = std::max(0.0f, shake_trauma - shake_trauma_decay * dt);

          if (shake_trauma > 0.0f) {
            // High-frequency sample updates for a crunchy quake feel.
            shake_sample_timer -= dt;
            if (shake_sample_timer <= 0.0f) {
              shake_sample_timer = 1.0f / 95.0f; // ~95Hz sample changes

              f32 sx = Engine::Random::random_range(-1.0f, 1.0f);
              f32 sy = Engine::Random::random_range(-1.0f, 1.0f);
              f32 mag = std::sqrt(sx * sx + sy * sy);
              if (mag > 0.001f) {
                sx /= mag;
                sy /= mag;
              }
              else {
                sx = 1.0f;
                sy = 0.0f;
              }
              shake_sample = Math::Vec2D(sx, sy);
            }

            const f32 quake = shake_trauma * shake_trauma; // Non-linear falloff (classic trauma shape)
            const f32 max_offset = shake_max_offset * quake;

            // Slightly stronger horizontal motion reads better in platformers.
            render_camera.position.x += shake_sample.x * max_offset;
            render_camera.position.y += shake_sample.y * (max_offset * 0.80f);
          }
        }

        Engine::Graphics::set_camera(render_camera);
      }

      static void add_shake(f32 trauma) {
        if (trauma <= 0.0f) {
          Log::warning("camera shake rejected: invalid trauma={}", trauma);
          return;
        }

        Log::info("camera shake accepted: trauma={}", trauma);

        shake_trauma = std::min(0.3f, shake_trauma + trauma);
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

        shake_trauma = 0.0f;
        shake_sample_timer = 0.0f;
        shake_sample = { 0.0f, 0.0f };
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
        bool camera_modified = false;

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
          camera_modified = true;
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
          camera_modified = true;
        }

        if (camera_modified) {
          Engine::Graphics::set_camera(camera);
        }
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


  }; // namespace Systems
} // namespace Engine
