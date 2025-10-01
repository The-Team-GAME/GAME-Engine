#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include <iostream>

class RectRenderSystem : public ECS::System {
public:
  RectRenderSystem() {
    require_components<Components::Color, Components::Position, Components::Size>();
  }

  void draw_entity(const ECS::Entity& entity) override {
    auto* color = entity.get_component<Components::Color>();
    auto* pos = entity.get_component<Components::Position>();
    auto* size = entity.get_component<Components::Size>();

    auto rotation = entity.has_component<Components::Rotation>() ?
      *entity.get_component<Components::Rotation>() : Components::Rotation{ 0.0f };

    Engine::GRAPHICS.draw_rect({ color->X(), color->Y(), color->Z(), color->W()},
      pos->X(), pos->Y(), size->width, size->height,
      rotation.angle, Graphics::Origin::Center);
  }
};

class StaticSpriteRenderSystem : public ECS::System {
public:
  StaticSpriteRenderSystem() {
    require_components<Components::StaticSprite, Components::Position, Components::Size>();
  }

  void draw_entity(const ECS::Entity& entity) override {
    auto* sprite = entity.get_component<Components::StaticSprite>();
    auto* size = entity.get_component<Components::Size>();
    auto* pos = entity.get_component<Components::Position>();

    auto rotation = entity.has_component<Components::Rotation>() ?
      *entity.get_component<Components::Rotation>() : Components::Rotation{ 0.0f };
    auto scale = entity.has_component<Components::Scale>() ?
      *entity.get_component<Components::Scale>() : Components::Scale{ 1.0f, 1.0f };
    auto color = entity.has_component<Components::Color>() ?
      *entity.get_component<Components::Color>() : Components::Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    Engine::GRAPHICS.draw_texture(sprite->texture_id, pos->X(), pos->Y(),
      size->width * scale.X(), size->height * scale.Y(),
      rotation.angle, Graphics::Origin::Center,
      { color.X(), color.Y(), color.Z(), color.W() });
  }
};

class AnimatedSpriteRenderSystem : public ECS::System {
public:
  AnimatedSpriteRenderSystem() {
    require_components<Components::AnimatedSprite, Components::Position, Components::Size>();
  }

  void update_entity(ECS::Entity& entity) override {
    auto* sprite = entity.get_component<Components::AnimatedSprite>();

    if (sprite->isPlaying) {
      sprite->animationTime += Engine::Time::get_fixed_dt();
    }
  }

  void draw_entity(const ECS::Entity& entity) override {
    auto* sprite = entity.get_component<Components::AnimatedSprite>();
    auto* size = entity.get_component<Components::Size>();
    auto* pos = entity.get_component<Components::Position>();

    auto rotation = entity.has_component<Components::Rotation>() ?
      *entity.get_component<Components::Rotation>() : Components::Rotation{ 0.0f };
    auto scale = entity.has_component<Components::Scale>() ?
      *entity.get_component<Components::Scale>() : Components::Scale{ 1.0f, 1.0f };
    auto color = entity.has_component<Components::Color>() ?
      *entity.get_component<Components::Color>() : Components::Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    Engine::GRAPHICS.draw_animated_sprite(sprite->spriteSheetId, pos->X(), pos->Y(),
      size->width * scale.X(), size->height * scale.Y(),
      sprite->animationTime,
      rotation.angle, Graphics::Origin::Center,
      { color.X(), color.Y(), color.Z(), color.W() });
  }
};
