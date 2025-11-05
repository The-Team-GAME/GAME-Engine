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
        require_components<Components::Clickable, Components::Transform2D>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* clickable = entity.get_component<Components::Clickable>();
        auto* tr = entity.get_component<Components::Transform2D>();

        if (!clickable || !tr) return;

        const auto mouse = Engine::Input::get_mouse_position();

        // Move Transform2D.position to center;
        // scale.x/scale.y as width/height
        const float half_w = tr->scale.x * 0.5f;
        const float half_h = tr->scale.y * 0.5f;
        const float left = tr->position.x - half_w;
        const float right = tr->position.x + half_w;
        const float top = tr->position.y - half_h;
        const float bottom = tr->position.y + half_h;

        const bool inside =
          (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);

        // Prevents spamming every frame
        if (inside && Engine::Input::is_pressed(KeyCode::MouseLeft)) {

          //test shows mouse position and what entity pressed
          //std::cout << "[CLICK] mouse=(" << mouse.x << ", " << mouse.y << ")  entity=" << entity.get_id() << "\n";

          clickable->on_click();
        }
      }
    };

  } // namespace Systems
} // namespace Engine