#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

// For now this just clears into a solid color, but could be extended to do more complex background rendering
class BackgroundSystem : public ECS::System {
public:
  BackgroundSystem() {
    requireComponents<Components::Color, Components::Background>();
  }

  void updateEntity(ECS::EntityID entityId) override {
    auto* color = world->getComponent<Components::Color>(entityId);
    Engine::GRAPHICS->clearScreen(color->x, color->y, color->z, color->w);
  }
};