#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../Graphics/Renderer.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include <imgui.h>

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
        exclude_components<Components::Gizmo, Components::Text>(); // Only process entities without Gizmo and without Text
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        if (!tr) return;

        // Don't select entities when Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        if (ctrl_held) return;

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

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
          // Send selection event with start_dragging = true for immediate drag
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity.get_id(), true);
        }
      }
    };

    // System to select text entities when clicked
    class GizmoTextSelectionSystem : public Engine::ECS::System {
    public:
      GizmoTextSelectionSystem() {
        require_components<Components::Transform2D, Components::Text, Components::Font>();
        exclude_components<Components::Gizmo>(); // Only process entities without Gizmo
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* tr = entity.get_component<Components::Transform2D>();
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();

        if (!tr || !text || !font) return;

        // Don't select entities when Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        if (ctrl_held) return;

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

        // Accurately measure text dimensions
        // Text scale is calculated as (scale.x + scale.y) * 0.5
        float text_scale = (tr->scale.x + tr->scale.y) * 0.5f;
        Math::Vec2D text_size = Engine::Graphics::measure_text(font->font_handle, text->content, text_scale);

        float text_width = text_size.x;
        float text_height = text_size.y;

        // Use text dimensions for bounding box (top-left aligned)
        const float left = tr->position.x;
        const float right = tr->position.x + text_width;
        const float top = tr->position.y;
        const float bottom = tr->position.y + text_height;

        const bool inside =
          (mouse.x >= left) && (mouse.x <= right) &&
          (mouse.y >= top) && (mouse.y <= bottom);

        // Add Gizmo component when clicked
        if (inside && Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          // Send selection event with start_dragging = true for immediate drag
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity.get_id(), true);
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

        // Check if Ctrl is held (camera control mode)
        bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
          Engine::Input::is_held(Engine::KeyCode::CtrlRight);

        // If Ctrl is held, stop any ongoing drag but keep the gizmo visible
        if (ctrl_held) {
          gizmo->is_dragging = false;
          return;
        }

        const auto mouse_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

        if (!mouse_opt.has_value()) return;

        const auto mouse = mouse_opt.value();

        // Check if this is a text entity
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();

        bool inside = false;

        if (text && font) {
          // Calculate bounds for text entity (top-left aligned)
          float text_scale = (tr->scale.x + tr->scale.y) * 0.5f;
          Math::Vec2D text_size = Engine::Graphics::measure_text(font->font_handle, text->content, text_scale);

          const float left = tr->position.x;
          const float right = tr->position.x + text_size.x;
          const float top = tr->position.y;
          const float bottom = tr->position.y + text_size.y;

          inside = (mouse.x >= left) && (mouse.x <= right) &&
            (mouse.y >= top) && (mouse.y <= bottom);
        }
        else {
          // Calculate bounds for sprite entity (centered)
          const float half_w = tr->scale.x * 0.5f;
          const float half_h = tr->scale.y * 0.5f;
          const float left = tr->position.x - half_w;
          const float right = tr->position.x + half_w;
          const float top = tr->position.y - half_h;
          const float bottom = tr->position.y + half_h;

          inside = (mouse.x >= left) && (mouse.x <= right) &&
            (mouse.y >= top) && (mouse.y <= bottom);
        }

        // Handle starting drag on click
        if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
          if (inside) {
            // Start dragging - calculate offset from entity center to mouse position
            gizmo->drag_offset = Math::Vec2D(mouse.x - tr->position.x, mouse.y - tr->position.y);
            gizmo->is_dragging = true;
          }
          else {
            // Only deselect if click was NOT on an ImGui window
            if (!ImGui::GetIO().WantCaptureMouse) {
              // Clicked outside - deselect (send deselection event)
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
              return;
            }
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

        // Check if this is a text entity
        auto* text = entity.get_component<Components::Text>();
        auto* font = entity.get_component<Components::Font>();

        if (text && font) {
          // Draw outline for text entity using accurate text bounds
          float text_scale = (tr->scale.x + tr->scale.y) * 0.5f;
          Math::Vec2D text_size = Engine::Graphics::measure_text(font->font_handle, text->content, text_scale);

          float text_width = text_size.x;
          float text_height = text_size.y;

          // Draw outline at text position (top-left aligned)
          Math::Vec2D outline_center(tr->position.x + text_width * 0.5f, tr->position.y + text_height * 0.5f);
          Math::Vec2D outline_size(text_width, text_height);

          Engine::Graphics::draw_rect_outline(
            outline_center,
            outline_size,
            0.0f, // Text doesn't rotate
            gizmo->outline_thickness,
            gizmo->outline_color
          );
        }
        else {
          // Draw outline for regular sprite entity
          Engine::Graphics::draw_rect_outline(
            tr->position,
            tr->scale,
            tr->rotation,
            gizmo->outline_thickness,
            gizmo->outline_color
          );
        }
      }
    };

  } // namespace Systems
} // namespace Engine
