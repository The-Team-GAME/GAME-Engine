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
        exclude_components<Components::TextureAtlas>();
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

        if (!animation->isPlaying) return; 

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

    class CameraControlSystem : public Engine::ECS::System {
    private:
      bool is_dragging = false;
      glm::vec2 last_mouse_pos = { 0.0f, 0.0f };

    public:
      CameraControlSystem() {
        require_components<Components::Camera>();
      }

      void update() override {
        // Check if Ctrl is held
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        // Get current mouse position
        glm::vec2 current_mouse_pos = Engine::Input::get_mouse_position();

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

        // Perform camera drag
        if (is_dragging && ctrl_held && mouse_held) {
          // Calculate mouse delta
          glm::vec2 delta = current_mouse_pos - last_mouse_pos;
          last_mouse_pos = current_mouse_pos;

          // Find the active camera and update its position
          Engine::ECS::Entity active_camera = get_active_camera();
          if (active_camera.is_valid()) {
            auto* camera = active_camera.get_component<Components::Camera>();
            if (camera) {
              // Move camera in opposite direction of mouse drag for intuitive feel
              // Also account for camera zoom
              camera->position.x -= delta.x / camera->zoom;
              camera->position.y -= delta.y / camera->zoom;
            }
          }
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


  } // namespace Systems
} // namespace Engine
