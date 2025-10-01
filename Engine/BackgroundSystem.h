#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

// For now this just clears into a solid color, but could be extended to do more complex background rendering
class BackgroundSystem : public ECS::System {
public:
  BackgroundSystem() {
    requireComponents<Components::Color, Components::Background>();
  }

  void draw_entity(const ECS::Entity& entity) override {
    auto* color = entity.getComponent<Components::Color>();
    Engine::GRAPHICS.clearScreen(color->X(), color->Y(), color->Z(), color->W());
  }
};