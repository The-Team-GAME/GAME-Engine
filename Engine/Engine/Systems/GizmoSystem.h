#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../Graphics/Renderer.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"

// TODO: M2: NEED TO WORK WITH THINGS WITH RIGIDBODY

namespace Engine
{
  namespace Systems
  {
    // System to add Gizmo component when entities with Transform2D are clicked
    class GizmoSelectionSystem : public Engine::ECS::System {
    public:
      GizmoSelectionSystem() {
        require_components<Components::Transform2D>();
        exclude_components<Components::Gizmo>(); // Only process entities without Gizmo
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        if (!tr) return;

        // Don't select entities when Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        if (ctrl_held) return;

        const auto mouse = Engine::Input::get_mouse_world_position();

        // Check if mouse is inside entity bounds
        const float half_w = tr->scale.x * 0.5f;
        const float half_h = tr->scale.y * 0.5f;
        const float left = tr->position.x - half_w;
        const float right = tr->position.x + half_w;
        const float top = tr->position.y - half_h;
        const float bottom = tr->position.y + half_h;

        const bool inside =
          (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);

        // Add Gizmo component when clicked
        if (inside && Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          // Send selection event - this will add Gizmo component via the event listener
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity.get_id());
        }
      }
    };

    // System to handle dragging and drawing selected entities
    class GizmoManipulationSystem : public Engine::ECS::System {
    public:
      GizmoManipulationSystem() {
        require_components<Components::Transform2D, Components::Gizmo>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* gizmo = entity.get_component<Components::Gizmo>();

        if (!tr || !gizmo) return;

        // Initialize drag offset if not set (first frame after selection)
        if (gizmo->drag_offset.x == 0.0f && gizmo->drag_offset.y == 0.0f && !gizmo->is_dragging) {
          const auto mouse = Engine::Input::get_mouse_world_position();
          gizmo->drag_offset = Math::Vec2D(mouse.x - tr->position.x, mouse.y - tr->position.y);
        }

        // Check if Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        // If Ctrl is held, stop any ongoing drag but keep the gizmo visible
        if (ctrl_held) {
          gizmo->is_dragging = false;
          return;
        }

        const auto mouse = Engine::Input::get_mouse_world_position();

        // Calculate bounds
        const float half_w = tr->scale.x * 0.5f;
        const float half_h = tr->scale.y * 0.5f;
        const float left = tr->position.x - half_w;
        const float right = tr->position.x + half_w;
        const float top = tr->position.y - half_h;
        const float bottom = tr->position.y + half_h;

        const bool inside =
          (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);

        // Handle starting drag on click
        if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          if (inside) {
            // Start dragging - calculate offset from entity center to mouse position
            gizmo->drag_offset = Math::Vec2D(mouse.x - tr->position.x, mouse.y - tr->position.y);
            gizmo->is_dragging = true;
          }
          else {
            // Clicked outside - deselect (send deselection event)
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
            return;
          }
        }

        // Handle dragging
        if (gizmo->is_dragging) {
          if (Engine::Input::is_held(KeyCode::MouseLeft)) {
            // Update position while dragging (accounting for offset)
            tr->position.x = mouse.x - gizmo->drag_offset.x;
            tr->position.y = mouse.y - gizmo->drag_offset.y;
          }
          else {
            // Stop dragging when mouse is released
            gizmo->is_dragging = false;
          }
        }
      }

      void draw_entity(const Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* gizmo = entity.get_component<Components::Gizmo>();

        if (!tr || !gizmo) return;

        // Draw outline around selected entity
        Engine::Graphics::draw_rect_outline(
          tr->position,
          tr->scale,
          tr->rotation,
          gizmo->outline_thickness,
          gizmo->outline_color
        );
      }
    };

  } // namespace Systems
} // namespace Engine
