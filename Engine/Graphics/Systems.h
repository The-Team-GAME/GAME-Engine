#pragma once

#include "../ECS/ECS.h"
#include "Renderer.h"
#include "../Components.h"

namespace Systems
{

  // Dont use this, instead just pass in a Engine::Resources::get_unit_rect_texture() when using StaticSpriteRenderSystem
  //class RectRenderSystem : public ECS::System {
  //public:
  //  RectRenderSystem() {
  //    require_components<Components::Transform2D, Components::Color>();
  //  }

  //  void draw_entity(const ECS::Entity& entity) override {
  //    auto* transform = entity.get_component<Components::Transform2D>();
  //    auto* color = entity.get_component<Components::Color>();

  //    Engine::Graphics::draw_texture(
  //      Engine::Resources::get_unit_rect_texture(),
  //      Math::Vec2D(transform->position.x, transform->position.y),
  //      Math::Vec2D(transform->scale.x, transform->scale.y),
  //      transform->rotation,
  //      color->color
  //    );
  //  }
  //};

  class StaticSpriteRenderSystem : public ECS::System {
  public:
    StaticSpriteRenderSystem() {
      require_components<Components::Transform2D, Components::Color, Components::Texture>();
    }

    void draw_entity(const ECS::Entity& entity) override {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* color = entity.get_component<Components::Color>();
      auto* texture = entity.get_component<Components::Texture>();

      Engine::Graphics::draw_texture(
        texture->texture_handle,
        Math::Vec2D(transform->position.x, transform->position.y),
        Math::Vec2D(transform->scale.x, transform->scale.y),
        transform->rotation,
        color->color
      );
    }
  };


  class StaticSpriteRenderSystemNoColor : public ECS::System {
  public:
    StaticSpriteRenderSystemNoColor() {
      require_components<Components::Transform2D, Components::Texture>();
    }

    void draw_entity(const ECS::Entity& entity) override {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* texture = entity.get_component<Components::Texture>();

      Engine::Graphics::draw_texture(
        texture->texture_handle,
        Math::Vec2D(transform->position.x, transform->position.y),
        Math::Vec2D(transform->scale.x, transform->scale.y),
        transform->rotation,
        Math::Vec4D(255.0f, 255.0f, 255.0f, 255.0f) // White default
      );
    }
  };


  class AtlasSpriteRenderSystem : public ECS::System {
  public:
    AtlasSpriteRenderSystem() {
      require_components<Components::Transform2D, Components::Color, Components::Texture, Components::TextureAtlas>();
    }

    void draw_entity(const ECS::Entity& entity) override {
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
          color->color
        );
      }
    }
  };


  class AtlasSpriteRenderSystemNoColor : public ECS::System {
  public:
    AtlasSpriteRenderSystemNoColor() {
      require_components<Components::Transform2D, Components::Texture, Components::TextureAtlas>();
    }

    void draw_entity(const ECS::Entity& entity) override {
      auto* transform = entity.get_component<Components::Transform2D>();
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
          Math::Vec4D(255.0f, 255.0f, 255.0f, 255.0f) // White default
        );
      }
    }
  };


  class TextRenderSystem : public ECS::System {
  public:
    TextRenderSystem() {
      require_components<Components::Transform2D, Components::Color, Components::Font, Components::Text>();
    }

    void draw_entity(const ECS::Entity& entity) override {
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
        color->color
      );
    }
  };


  class TextRenderSystemNoColor : public ECS::System {
  public:
    TextRenderSystemNoColor() {
      require_components<Components::Transform2D, Components::Font, Components::Text>();
    }

    void draw_entity(const ECS::Entity& entity) override {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* font = entity.get_component<Components::Font>();
      auto* text = entity.get_component<Components::Text>();

      // Use the average of scale.x and scale.y as the text scale
      float text_scale = (transform->scale.x + transform->scale.y) * 0.5f;

      Engine::Graphics::draw_text(
        font->font_handle,
        text->content,
        Math::Vec2D(transform->position.x, transform->position.y),
        text_scale,
        Math::Vec4D(255.0f, 255.0f, 255.0f, 255.0f) // White default
      );
    }
  };


  class AnimationUpdateSystem : public ECS::System {
  public:
    AnimationUpdateSystem() {
      require_components<Components::Animation, Components::TextureAtlas>();
    }

    void update_entity(ECS::Entity& entity) override {
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
      size_t current_frame = 0;
      for (size_t i = 0; i < animation->frames.size(); ++i) {
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


} // namespace Systems