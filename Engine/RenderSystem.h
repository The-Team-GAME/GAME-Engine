#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include <iostream>

class RectRenderSystem : public ECS::System {
public:
  RectRenderSystem() {
    requireComponents<Components::Color, Components::Position, Components::Size>();
  }

  void drawEntity(const ECS::Entity& entity) override {
    auto* color = entity.getComponent<Components::Color>();
    auto* pos = entity.getComponent<Components::Position>();
    auto* size = entity.getComponent<Components::Size>();

    auto rotation = entity.hasComponent<Components::Rotation>() ?
      *entity.getComponent<Components::Rotation>() : Components::Rotation{ 0.0f };

    Engine::GRAPHICS.drawRect({ color->X(), color->Y(), color->Z(), color->W()},
      pos->X(), pos->Y(), size->width, size->height,
      rotation.angle, Graphics::Origin::Center);
  }
};

class StaticSpriteRenderSystem : public ECS::System {
public:
  StaticSpriteRenderSystem() {
    requireComponents<Components::StaticSprite, Components::Position, Components::Size>();
  }

  void drawEntity(const ECS::Entity& entity) override {
    auto* sprite = entity.getComponent<Components::StaticSprite>();
    auto* size = entity.getComponent<Components::Size>();
    auto* pos = entity.getComponent<Components::Position>();

    auto rotation = entity.hasComponent<Components::Rotation>() ?
      *entity.getComponent<Components::Rotation>() : Components::Rotation{ 0.0f };
    auto scale = entity.hasComponent<Components::Scale>() ?
      *entity.getComponent<Components::Scale>() : Components::Scale{ 1.0f, 1.0f };
    auto color = entity.hasComponent<Components::Color>() ?
      *entity.getComponent<Components::Color>() : Components::Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    Engine::GRAPHICS.drawTexture(sprite->textureId, pos->X(), pos->Y(),
      size->width * scale.X(), size->height * scale.Y(),
      rotation.angle, Graphics::Origin::Center,
      { color.X(), color.Y(), color.Z(), color.W() });
  }
};
