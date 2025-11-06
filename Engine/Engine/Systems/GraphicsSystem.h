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
      Engine::Graphics::Camera camera{}; // this is the camera that will be sent to the renderer

    public:
      CameraControlSystem() {
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

        // Apply state changes when dirty flag is set
        if (anim_state->dirty) {
          // Log::info("Updating animation state for entity: " + std::to_string(entity.get_id()));
          auto it = anim_state->states.find(anim_state->current_state);
          if (it != anim_state->states.end()) {
            // Log::info("Found animation state: " + anim_state->current_state);
            const auto& state_data = it->second;
            texture->texture_handle = state_data.texture.texture_handle;
            atlas->subTextures = state_data.texture_atlas.subTextures;
            atlas->drawIndex = state_data.texture_atlas.drawIndex;
            animation->frames = state_data.animation.frames;
            animation->startTime = Engine::Time::get_time(); // Reset start time to NOW when transitioning
            animation->totalDuration = state_data.animation.totalDuration;

            // Apply scale if provided
            if (state_data.scale.has_value()) {
              auto* transform = entity.get_component<Components::Transform2D>();
              if (transform) {
                transform->scale = state_data.scale.value();
              }
            }
          }
          anim_state->dirty = false; // Clear dirty flag after update
        }

        // Check for non-looping animation completion and auto-transition
        auto it = anim_state->states.find(anim_state->current_state);
        if (it != anim_state->states.end()) {
          const auto& state_data = it->second;

          // Only check for completion if animation is non-looping and has a next state
          if (!state_data.is_looping && state_data.next_state.has_value()) {
            // Check if animation has completed
            float elapsed = Engine::Time::get_time() - animation->startTime;
            if (elapsed >= animation->totalDuration) {
              // Animation finished, transition to next state
              anim_state->set_entity_state(state_data.next_state.value());
            }
          }
        }
      }
    };


  }; // namespace Systems
} // namespace Engine
