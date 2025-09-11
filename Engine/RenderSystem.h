#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include <iostream>

// TODO: to be removed, replaced by GameObjectRenderSystem
class RenderSystem : public ECS::System {
public:
  RenderSystem() {
    requireComponents<Components::Position, Components::Renderable>();
  }

  void updateEntity(ECS::EntityID entityId, float /*dt*/) override {
    auto* pos = world->getComponent<Components::Position>(entityId);
    //auto* render = world->getComponent<Components::Renderable>(entityId);

    /*std::cout << "Rendering sprite " << render->spriteId
      << " at (" << pos->x << ", " << pos->y
      << ") scale=" << render->scale << "\n";*/

    Engine::GRAPHICS->drawRect(pos->x, pos->y, 10.0f, 10.0f, { 1.0f, 1.0f, 1.0f, 1.0f }); // TODO: allow changing color
  }
};

class GameObjectRenderSystem : public ECS::System {
public:
  GameObjectRenderSystem() {
    requireComponents<Components::StaticSprite, Components::Size>();
  }

  void updateEntity(ECS::EntityID entityId, float /*dt*/) override {
    auto* sprite = world->getComponent<Components::StaticSprite>(entityId);
    auto* size = world->getComponent<Components::Size>(entityId);
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* rotation = world->getComponent<Components::Rotation>(entityId);
    auto* scale = world->getComponent<Components::Scale>(entityId);

    // Default values if components are missing
    float x = pos ? pos->x : 0.0f;
    float y = pos ? pos->y : 0.0f;
    float angle = rotation ? rotation->angle : 0.0f;
    float scaleX = scale ? scale->x : 1.0f;
    float scaleY = scale ? scale->y : 1.0f;

    Engine::GRAPHICS->drawTexture(sprite->textureId, x, y, size->width * scaleX, size->height * scaleY, angle);
  }
};
