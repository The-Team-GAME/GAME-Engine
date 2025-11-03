#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"

namespace Engine
{
  namespace Systems
  {

    class ClickableSystem : public Engine::ECS::System {
    public:
      ClickableSystem() {
        require_components<Components::Clickable>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* clickable = entity.get_component<Components::Clickable>();

        if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          clickable->on_click();
        }
      }
    };

  } // namespace Systems
} // namespace Engine