#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Graphics/Renderer.h"
#include "../InputManager.h"
namespace Engine
{
  namespace Systems
  {
    class StaticSpriteRenderSystem : public Engine::ECS::System {
    public:
      StaticSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture>();
        exclude_components<Components::TextureAtlas, Components::Distortion>();
      }

      void draw_entity(const Engine::ECS::Entity& entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();

        Engine::Graphics::draw_texture(
          texture->texture_handle,
          Math::Vec2D(transform->position.x, transform->position.y),
          Math::Vec2D(transform->scale.x, transform->scale.y),
          transform->rotation,
          *color
        );
      }
    };

    class AtlasSpriteRenderSystem : public Engine::ECS::System {
    public:
      AtlasSpriteRenderSystem() {
        require_components<Components::Transform2D, Components::Color, Components::Texture, Components::TextureAtlas>();
        exclude_components<Components::Distortion>();
      }

      void draw_entity(const Engine::ECS::Entity& entity) override {
        auto* transform = entity.get_component<Components::Transform2D>();
        auto* color = entity.get_component<Components::Color>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();

        // Ensure drawIndex is within bounds
        if (atlas->drawIndex < atlas->subTextures.size()) {
          const auto& subTexture = atlas->subTextures[atlas->drawIndex];

          Engine::Graphics::draw_subtexture(
            texture->texture_handle,
            subTexture.tex_coords,
            Math::Vec2D(transform->position.x, transform->position.y),
            Math::Vec2D(transform->scale.x, transform->scale.y),
            transform->rotation,
            *color
          );
        }
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

        // Loop the animation by wrapping elapsed time
        float wrapped_time = fmod(elapsed, animation->totalDuration);

        // Find which frame should be displayed based on elapsed time
        int current_frame = 0;
        for (int i = 0; i < animation->frames.size(); ++i) {
          if (wrapped_time >= animation->frames[i].before_time) {
            current_frame = i;
          }
          else {
            break;
          }
        }

        // Update the atlas drawIndex to the current frame
        atlas->drawIndex = current_frame;
      }
    };

    class CameraSystem : public Engine::ECS::System {
    public:
      CameraSystem() {
        require_components<Components::Camera>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* camera_component = entity.get_component<Components::Camera>();

        // If this camera is marked as active, apply it to the renderer
        if (camera_component->is_active) {
          Engine::Graphics::Camera renderer_camera;
          renderer_camera.position = camera_component->position;
          renderer_camera.rotation = camera_component->rotation;
          renderer_camera.zoom = camera_component->zoom;
          Engine::Graphics::set_camera(renderer_camera);
        }
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

        Engine::Graphics::draw_texture_distortion(
          texture->texture_handle,
          Math::Vec2D(transform->position.x, transform->position.y),
          Math::Vec2D(transform->scale.x, transform->scale.y),
          transform->rotation,
          *color,
          distortion->intensity,
          distortion->speed,
          distortion->mesh_intensity
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
            distortion->mesh_intensity
          );
        }
      }
    };

    class CameraControlSystem : public Engine::ECS::System {
    private:
      bool is_dragging = false;
      glm::vec2 last_mouse_pos = { 0.0f, 0.0f };
      Engine::Graphics::Camera free_cam_state; // Free camera state when no entity camera is active

    public:
      CameraControlSystem() {
        // No component requirements - this system works globally
      }

      void update() override {
        // Check if Ctrl is held
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        // Get current mouse position
        glm::vec2 current_mouse_pos = Engine::Input::get_mouse_position();

        // Get scroll offset
        glm::vec2 scroll = Engine::Input::get_scroll_offset();

        // Check if left mouse button is held
        bool mouse_held = Engine::Input::is_held(Engine::KeyCode::MouseLeft);

        // Start dragging
        if (ctrl_held && Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
          is_dragging = true;
          last_mouse_pos = current_mouse_pos;
        }

        // Stop dragging
        if (!mouse_held || !ctrl_held) {
          is_dragging = false;
        }

        // Find the active camera entity
        Engine::ECS::Entity active_camera = get_active_camera();
        bool has_active_camera = active_camera.is_valid();

        // Handle zoom with Ctrl + Scroll
        if (ctrl_held && scroll.y != 0.0f) {
          //std::cout << "Zooming! Ctrl held: " << ctrl_held << ", Scroll Y: " << scroll.y << "\n";
          float zoom_speed = 0.1f;
          float zoom_delta = scroll.y * zoom_speed;

          if (has_active_camera) {
            auto* camera = active_camera.get_component<Components::Camera>();
            if (camera) {
              camera->zoom += zoom_delta;
              // Clamp zoom to reasonable values
              camera->zoom = glm::clamp(camera->zoom, 0.1f, 10.0f);
            }
          }
          else {
            // Free cam mode - adjust free cam zoom
            free_cam_state.zoom += zoom_delta;
            free_cam_state.zoom = glm::clamp(free_cam_state.zoom, 0.1f, 10.0f);
          }
        }

        // Perform camera drag
        if (is_dragging && ctrl_held && mouse_held) {
          // Calculate mouse delta
          glm::vec2 delta = current_mouse_pos - last_mouse_pos;
          last_mouse_pos = current_mouse_pos;

          if (has_active_camera) {
            // Move the active camera entity using its Camera component
            auto* camera = active_camera.get_component<Components::Camera>();
            if (camera) {
              // Move camera in opposite direction of mouse drag for intuitive feel
              // Also account for camera zoom
              camera->position.x -= delta.x / camera->zoom;
              camera->position.y -= delta.y / camera->zoom;
            }
          }
          else {
            // Free cam mode - move the free cam state
            free_cam_state.position.x -= delta.x / free_cam_state.zoom;
            free_cam_state.position.y -= delta.y / free_cam_state.zoom;
          }
        }

        // Sync free cam state with active camera, or apply it to renderer if no active camera
        if (has_active_camera) {
          // Update free cam state to match the active camera
          auto* camera = active_camera.get_component<Components::Camera>();
          if (camera) {
            free_cam_state.position = camera->position;
            free_cam_state.rotation = camera->rotation;
            free_cam_state.zoom = camera->zoom;
          }
        }
        else {
          // Apply free cam state to renderer
          Engine::Graphics::set_camera(free_cam_state);
        }
      }

    private:
      Engine::ECS::Entity get_active_camera() {
        Engine::ECS::Entity result(Engine::ECS::INVALID_ENTITY);
        Engine::ECS::for_each_entity_with<Components::Camera>([&](Engine::ECS::Entity entity) {
          auto* camera = entity.get_component<Components::Camera>();
          if (camera && camera->is_active) {
            result = entity;
          }
          });
        return result;
      }
    };


    class AnimationStateSystem : public Engine::ECS::System {
    public:
      AnimationStateSystem() {
        require_components<Components::AnimationState, Components::Texture, Components::TextureAtlas, Components::Animation>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* anim_state = entity.get_component<Components::AnimationState>();
        auto* texture = entity.get_component<Components::Texture>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();
        auto* animation = entity.get_component<Components::Animation>();

        if (anim_state->dirty) { // Update only if state is marked dirty
          // Log::info("Updating animation state for entity: " + std::to_string(entity.get_id()));
          auto it = anim_state->states.find(anim_state->current_state);
          if (it != anim_state->states.end()) {
            // Log::info("Found animation state: " + anim_state->current_state);
            const auto& state_data = it->second;
            texture->texture_handle = state_data.texture.texture_handle;
            atlas->subTextures = state_data.texture_atlas.subTextures;
            atlas->drawIndex = state_data.texture_atlas.drawIndex;
            animation->frames = state_data.animation.frames;
            animation->startTime = state_data.animation.startTime;
            animation->totalDuration = state_data.animation.totalDuration;
          }
          anim_state->dirty = false; // Clear dirty flag after update
        }
      }
    };


  }; // namespace Systems
} // namespace Engine
