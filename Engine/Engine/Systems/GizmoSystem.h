#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../Graphics/Renderer.h"

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

        const auto mouse = Engine::Input::get_mouse_position();

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
          // Calculate offset from entity center to mouse position
          Math::Vec2D offset = Math::Vec2D(mouse.x - tr->position.x, mouse.y - tr->position.y);

          entity.add_component<Components::Gizmo>(
            Math::Vec4D(0.0f, 1.0f, 0.0f, 1.0f), // Green outline
            2.0f                                   // Thickness
          );

          // Set the drag offset
          auto* gizmo = entity.get_component<Components::Gizmo>();
          if (gizmo) {
            gizmo->drag_offset = offset;
            gizmo->is_dragging = true;
          }
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

        const auto mouse = Engine::Input::get_mouse_position();

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

        // Deselect (remove Gizmo) when clicking outside
        if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          const float half_w = tr->scale.x * 0.5f;
          const float half_h = tr->scale.y * 0.5f;
          const float left = tr->position.x - half_w;
          const float right = tr->position.x + half_w;
          const float top = tr->position.y - half_h;
          const float bottom = tr->position.y + half_h;

          const bool inside =
            (mouse.x >= left) && (mouse.x <= right) &&
            (mouse.y >= top) && (mouse.y <= bottom);

          if (!inside) {
            entity.remove_component<Components::Gizmo>();
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
