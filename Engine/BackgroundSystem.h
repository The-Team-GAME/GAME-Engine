#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

// For now this just clears into a solid color, but could be extended to do more complex background rendering
class BackgroundSystem : public ECS::System {
public:
  BackgroundSystem() {
    require_components<Components::Color, Components::Background>();
  }

  void draw_entity(const ECS::Entity& entity) override {
    auto* color = entity.get_component<Components::Color>();
    Engine::GRAPHICS.clear_screen(color->X(), color->Y(), color->Z(), color->W());
  }
};