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
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

        const float LERP_SPEED = 0.15f;
        const float CLAMP = 0.01f;

				//Sets the original and target scale
        if (clickable->original_scale.x == 0.0f) {
          clickable->original_scale = tr->scale;
          clickable->target_scale = clickable->original_scale * 0.7f;
        }

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

        
        if (inside) {
          if (!clickable->is_hovered) { // If we just entered the clickable area, set the state to hovered
            clickable->is_hovered = true;
          }
        }
        else { // If we just left the clickable area, set the state to un-hovered
          if (clickable->is_hovered) {
            clickable->is_hovered = false;
          }
        }
        if (clickable->is_hovered) { //SCALE + LERP
          tr->scale.x = tr->scale.x + (clickable->target_scale.x - tr->scale.x) * LERP_SPEED;
          tr->scale.y = tr->scale.y + (clickable->target_scale.y - tr->scale.y) * LERP_SPEED;

          // Clamp
          if (std::abs(tr->scale.x - clickable->target_scale.x) < CLAMP) {
            tr->scale = clickable->target_scale;
          }
        }
				else { //DESCALE + LERP
          tr->scale.x = tr->scale.x + (clickable->original_scale.x - tr->scale.x) * LERP_SPEED;
          tr->scale.y = tr->scale.y + (clickable->original_scale.y - tr->scale.y) * LERP_SPEED;

          // Clamp
          if (std::abs(tr->scale.x - clickable->original_scale.x) < CLAMP) {
            tr->scale = clickable->original_scale;
          }
        }

				// mouse click detection
        if (inside) {
          if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
            clickable->on_click();
          }
        }
      }
    };

  } // namespace Systems
} // namespace Engine