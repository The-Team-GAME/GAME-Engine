//-----------------------------------------------------------------------------------
// file:    UISystem.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (40%)
//          Primary author
// 
//          Ryan Ang (60%)
//          Secondary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines a Event-driven Clickable System that checks for entities
//          that has the clickable component and gets the mouse position of when the
//          object was clicked. (Ryan) It also handles the dynamic scaling animation when the mouse
//          hovers over the clickable entity.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


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

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) {
          return;
        }

        const Math::Vec2D& mouse = mouse_opt.value();

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


				// mouse click detection
        if (inside) {
          clickable->on_hover(entity);

          if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
            clickable->on_click();
          }
        }
        else {
          clickable->exit_hover(entity);
        }
      }
    };

  } // namespace Systems
} // namespace Engine