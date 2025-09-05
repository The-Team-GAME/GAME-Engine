#pragma once
#include "../ECS/System.h"
#include "../Components/BasicComponents.h"
#include <iostream>

class RenderSystem : public ECS::System {
public:
  RenderSystem() {
    requireComponents<Components::Position, Components::Renderable>();
  }

  void updateEntity(ECS::EntityID entityId, float deltaTime) override {
    auto* pos = world->getComponent<Components::Position>(entityId);
    auto* render = world->getComponent<Components::Renderable>(entityId);

    /*std::cout << "Rendering sprite " << render->spriteId
      << " at (" << pos->x << ", " << pos->y
      << ") scale=" << render->scale << "\n";*/

    Graphics::drawRect(pos->x, pos->y, 10.0f, 10.0f, { 1.0f, 1.0f, 1.0f, 1.0f }); // TODO: allow changing color
  }
};
