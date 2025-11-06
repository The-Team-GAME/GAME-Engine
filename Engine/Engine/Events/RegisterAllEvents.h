#pragma once
#include "../EventManager/EventManager.h"
#include "WindowEvents.h"
#include "EngineEvents.h"
#include "../Graphics/Renderer.h"
#include "../Resources.h"
#include "../Components/GizmoComponents.h"


namespace Engine {
  namespace Events {
    void register_all_events() {
      // Register the graphics resize system 
      Engine::EventManager::add_event_listener<Engine::Events::WindowResizeEvent>([](const Engine::Events::WindowResizeEvent& event)
        {
          Engine::Graphics::set_viewport({ 0, 0, event.width, event.height });
        }
      );

      // Register file drop event to load resources
      Engine::EventManager::add_event_listener<Engine::Events::FileDropEvent>([](const Engine::Events::FileDropEvent& event)
        {
          for (const auto& filepath : event.filepaths)
          {
            Log::info("File dropped: " + filepath);
            Engine::Resources::load(filepath, false, true);
          }
        }
      );

      // Register entity selection event to sync gizmo system
      Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>([](const Engine::Events::SelectEntityEvent& event)
        {
          // Remove Gizmo from all entities first
          Engine::ECS::for_each_entity_with<Components::Gizmo>([](Engine::ECS::Entity entity) {
            entity.remove_component<Components::Gizmo>();
            });

          // Add Gizmo to the selected entity if valid
          if (event.entity_id != Engine::ECS::INVALID_ENTITY) {
            auto entity = Engine::ECS::get_entity(event.entity_id);
            if (entity.is_valid() && entity.has_component<Components::Transform2D>()) {
              entity.add_component<Components::Gizmo>(
                Math::Vec4D(0.0f, 1.0f, 0.0f, 1.0f), // Green outline
                2.0f                                   // Thickness
              );

              auto* gizmo = entity.get_component<Components::Gizmo>();

              // Initialize drag offset to zero - will be set on first update if needed
              if (gizmo) {
                gizmo->drag_offset = Math::Vec2D(0.0f, 0.0f);
                gizmo->is_dragging = false;
              }
            }
          }
        }
      );
    }
  }
}
