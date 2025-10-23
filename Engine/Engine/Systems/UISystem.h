#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Graphics/Renderer.h"
#include "../InputManager.h"

namespace Engine
{
  namespace Systems
  {

    class ButtonRenderSystem : public Engine::ECS::System {
    public:
      ButtonRenderSystem() {
        require_components<Components::Clickable>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* clickable = entity.get_component<Components::Clickable>();

        if (Engine::Input::is_mouse_button_pressed(Engine::MouseButton::Left)) {
          clickable->on_click();
        }
      }
    };

  } // namespace Systems
} // namespace Engine