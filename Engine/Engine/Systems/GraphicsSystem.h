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

      void draw_entity(const Engine::ECS::Entity& entity) override {
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

      void draw_entity(const Engine::ECS::Entity& entity) override {
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

      void draw_entity(const Engine::ECS::Entity& entity) override {
        auto* int_map = entity.get_component<Components::TileMap>();

        // Skip if tilemap is empty
        if (!int_map || int_map->is_empty()) {
          return;
        }

        // Get the atlas entries from project settings
        const auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get();

        // Determine the color based on tilemap mode and selected layer
        Math::Vec4D color(1.0f, 1.0f, 1.0f, 1.0f); // Default: white (full brightness)

        // Check if tilemap mode is enabled AND dimming is enabled
        if (TilemapPanel::is_tilemap_mode_enabled() && TilemapPanel::is_dim_non_selected_layers_enabled()) {
          // Get the selected layer name
          std::string selected_layer = TilemapPanel::get_selected_layer();

          // Check if this entity is on the selected layer
          bool on_selected_layer = entity.is_on_layer(selected_layer);

          if (!on_selected_layer) {
            // Darken tiles on non-selected layers
            color = Math::Vec4D(0.3f, 0.3f, 0.3f, 1.0f);
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

      void draw_entity(const Engine::ECS::Entity& entity) override {
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

      void update_entity(Engine::ECS::Entity& entity) override {
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
          std::cout << "Warning: current_frame out of bounds in AnimationUpdateSystem\n";
          atlas->drawIndex = 0; // Fallback to first frame
        }
      }
    };

    class CameraSystem : public Engine::ECS::System {
    private:
      inline static Engine::Graphics::Camera camera{}; // this is the camera that will be sent to the renderer

    public:
      CameraSystem() {
      }

      void update() override {
        Engine::ECS::for_each_entity_with<Components::Camera>(
          [this](Engine::ECS::Entity entity) {
            auto* camera_component = entity.get_component<Components::Camera>();

            // If this camera is marked as active, use its parameters
            if (camera_component->is_active) {
              camera.position = camera_component->position;
              camera.rotation = camera_component->rotation;
              camera.zoom = camera_component->zoom;
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

      void draw_entity(const Engine::ECS::Entity& entity) override {
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

      void draw_entity(const Engine::ECS::Entity& entity) override {
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
