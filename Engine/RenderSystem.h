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
    requireComponents<Components::StaticSprite, Components::Position, Components::Size, Components::Rotation>();
  }

  void updateEntity(ECS::EntityID entityId, float /*dt*/) override {
    auto* sprite = world->getComponent<Components::StaticSprite>(entityId);
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* size = world->getComponent<Components::Size>(entityId);
    auto* rotation = world->getComponent<Components::Rotation>(entityId);

    Engine::GRAPHICS->drawTexture(sprite->textureId, pos->x, pos->y, size->width, size->height, rotation->angle);
  }
};
