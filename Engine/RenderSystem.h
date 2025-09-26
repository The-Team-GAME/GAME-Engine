#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include <iostream>

class RectRenderSystem : public ECS::System {
public:
  RectRenderSystem() {
    requireComponents<Components::Color, Components::Position, Components::Size>();
  }
  void drawEntity(ECS::EntityID entityId) override {
    auto* color = world->getComponent<Components::Color>(entityId);
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* size = world->getComponent<Components::Size>(entityId);

    auto rotation = world->hasComponent<Components::Rotation>(entityId) ? *world->getComponent<Components::Rotation>(entityId) : Components::Rotation{ 0.0f };

    Engine::GRAPHICS.drawRect({ color->x, color->y, color->z, color->w }, pos->x, pos->y, size->width, size->height, rotation.angle, Graphics::Origin::Center);
  }
};

class StaticSpriteRenderSystem : public ECS::System {
public:
  StaticSpriteRenderSystem() {
    requireComponents<Components::StaticSprite, Components::Position, Components::Size>();
  }

  void drawEntity(ECS::EntityID entityId) override {
    auto* sprite = world->getComponent<Components::StaticSprite>(entityId);
    auto* size = world->getComponent<Components::Size>(entityId);
    auto* pos = world->getComponent<Components::Position>(entityId);

    auto rotation = world->hasComponent<Components::Rotation>(entityId) ? *world->getComponent<Components::Rotation>(entityId) : Components::Rotation{ 0.0f };
    auto scale = world->hasComponent<Components::Scale>(entityId) ? *world->getComponent<Components::Scale>(entityId) : Components::Scale{ 1.0f, 1.0f };
    auto color = world->hasComponent<Components::Color>(entityId) ? *world->getComponent<Components::Color>(entityId) : Components::Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    Engine::GRAPHICS.drawTexture(sprite->textureId, pos->x, pos->y, size->width * scale.x, size->height * scale.y, rotation.angle, Graphics::Origin::Center, { color.x, color.y, color.z, color.w });
  }
};
