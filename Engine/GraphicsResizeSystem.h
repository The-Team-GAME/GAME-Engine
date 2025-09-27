#pragma once
#include "ECS/ECS.h"
#include "BasicEvents.h"
#include "Core.h"

class GraphicsResizeSystem : public ECS::System {
public:
  GraphicsResizeSystem() {
    listenToEvents<WindowResizeEvent>();
    // This system doesn't require any specific components
  }

  void onEvent(const ECS::Event& event, ECS::EntityID entity) override {
    if (const auto* resizeEvent = event.get<WindowResizeEvent>()) {
      // Update the graphics viewport
      Engine::GRAPHICS.setViewport(resizeEvent->width, resizeEvent->height);
    }
  }
};