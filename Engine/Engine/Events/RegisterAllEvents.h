//-----------------------------------------------------------------------------------
// file:    RegisterAllEvents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Central event registration for the engine's event system.
//          Registers all core event listeners including window resize handling,
//          file drop processing for resource loading, and entity selection
//          synchronization between viewport and editor interfaces.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../EventManager/EventManager.h"
#include "WindowEvents.h"
#include "EngineEvents.h"
#include "../Graphics/Renderer.h"
#include "../Resources.h"
#include "../Components/GizmoComponents.h"
#include "../InputManager.h"
#include "../UndoManager/UndoManager.h"
#include "../ProjectSaveManager.h"
#include "../LevelManager.h"
#include "../FileIO/Settings.h"


namespace Engine {
  namespace Events {
    void register_all_events() {
      // Register the graphics resize system 
      Engine::EventManager::add_event_listener<Engine::Events::WindowResizeEvent>([](const Engine::Events::WindowResizeEvent& event)
        {
          // Determine whether we're rendering directly to the window
#ifdef GAME_BUILD
          bool direct_to_window = true;
#else
          bool direct_to_window = !Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode;
#endif

          if (direct_to_window) {
            // Letterbox / pillarbox: fit a 16:9 rectangle inside the window
            // so the game content is never stretched on non-16:9 screens.
            constexpr float target_aspect = 16.0f / 9.0f;
            float window_aspect = static_cast<float>(event.width) / static_cast<float>(event.height > 0 ? event.height : 1);

            int vp_x, vp_y, vp_w, vp_h;
            if (window_aspect > target_aspect) {
              // Window is wider than 16:9 -> pillarbox (black bars on sides)
              vp_h = event.height;
              vp_w = static_cast<int>(event.height * target_aspect);
              vp_x = (event.width - vp_w) / 2;
              vp_y = 0;
            }
            else {
              // Window is taller than 16:9 -> letterbox (black bars top/bottom)
              vp_w = event.width;
              vp_h = static_cast<int>(event.width / target_aspect);
              vp_x = 0;
              vp_y = (event.height - vp_h) / 2;
            }

            Engine::Graphics::set_viewport({ vp_x, vp_y, vp_w, vp_h });
            Engine::Graphics::set_virtual_resolution(1600.0f, 900.0f);
            Engine::Graphics::resize_post_processing(vp_w, vp_h);
          }
          else {
            // Panel viewport mode: let the viewport panel handle its own sizing
            Engine::Graphics::set_viewport({ 0, 0, event.width, event.height });
          }
        }
      );

      // Register file drop event to load resources
      Engine::EventManager::add_event_listener<Engine::Events::FileDropEvent>([](const Engine::Events::FileDropEvent& event)
        {
          for (const auto& filepath : event.filepaths)
          {
            Log::info("File dropped: {}", filepath);
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
              auto* transform = entity.get_component<Components::Transform2D>();

              entity.add_component<Components::Gizmo>(
                Math::Vec4D(0.0f, 1.0f, 0.0f, 1.0f), // Green outline
                2.0f                                   // Thickness
              );

              auto* gizmo = entity.get_component<Components::Gizmo>();

              if (gizmo && transform) {
                if (event.start_dragging) {
                  // UI entities use screen-space coordinates (not affected by camera)
                  const auto mouse = entity.has_component<Components::UI>()
                    ? Engine::Input::get_mouse_position()
                    : Engine::Input::get_mouse_world_position();
                  gizmo->drag_offset = Math::Vec2D(mouse.x - transform->position.x, mouse.y - transform->position.y);
                  gizmo->is_dragging = true;

                  // Set active handle to Body for immediate dragging
                  gizmo->active_handle = Components::GizmoHandleType::Body;
                  gizmo->hover_handle = Components::GizmoHandleType::Body;

                  // Cache initial state for dragging
                  gizmo->start_mouse_world = mouse;
                  gizmo->start_pos = transform->position;
                  gizmo->start_scale = transform->scale;
                  gizmo->start_rotation = transform->rotation;
                }
                else {
                  // Initialize drag offset to zero - will be set on first update if needed
                  gizmo->drag_offset = Math::Vec2D(0.0f, 0.0f);
                  gizmo->is_dragging = false;
                }
              }
            }
          }
        }
      );

      // Register generic component modification event (reflection-based)
      Engine::EventManager::add_event_listener<Engine::Events::ModifyComponentEvent>([](const Engine::Events::ModifyComponentEvent& event)
        {
          auto entity = Engine::ECS::get_entity(event.entity_id);
          if (!entity.is_valid()) {
            return;
          }

          // Get the component array for this component type
          auto& containers = Engine::ECS::Internal::get_containers();
          auto& comp_ptr = containers.component_arrays[event.component_type_id];
          if (!comp_ptr) {
            return;
          }

          Engine::ECS::IComponentArray* array = comp_ptr.get();

          // Apply the new component data using reflection
          array->set_component_from_generic_value(event.entity_id, event.new_data);

          // Record undo if requested
          if (event.record_undo) {
            Engine::ECS::EntityID entity_id = event.entity_id;
            Engine::ECS::ComponentTypeID component_type_id = event.component_type_id;
            ReflectionLibrary::GenericValue old_data = event.old_data;
            ReflectionLibrary::GenericValue new_data = event.new_data;

            Engine::UndoManager::record(
              // Do: Apply new data
              [entity_id, component_type_id, new_data]() {
                Engine::EventManager::send_event<Engine::Events::ModifyComponentEvent>(
                  entity_id, component_type_id, new_data, new_data, false
                );
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              },
              // Undo: Restore old data
              [entity_id, component_type_id, old_data, new_data]() {
                Engine::EventManager::send_event<Engine::Events::ModifyComponentEvent>(
                  entity_id, component_type_id, old_data, new_data, false
                );
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              }
            );

            // Mark level as dirty
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        }
      );

      // Register add component event (reflection-based)
      Engine::EventManager::add_event_listener<Engine::Events::AddComponentEvent>([](const Engine::Events::AddComponentEvent& event)
        {
          auto entity = Engine::ECS::get_entity(event.entity_id);
          if (!entity.is_valid()) {
            return;
          }

          // Get the component array for this component type
          auto& containers = Engine::ECS::Internal::get_containers();
          auto& comp_ptr = containers.component_arrays[event.component_type_id];
          if (!comp_ptr) {
            return;
          }

          Engine::ECS::IComponentArray* array = comp_ptr.get();

          // Add the component using reflection
          if (array->add_component_from_generic_value(event.entity_id, event.component_data)) {
            // Manually update signature and notify systems since add_component_from_generic_value doesn't do it
            containers.entity_signatures[event.entity_id].set(event.component_type_id);
            Engine::ECS::Internal::entity_component_changed(event.entity_id);
          }
          else {
            // If adding from generic value failed (e.g. empty data), try adding default component
            // add_default_component calls ECS::add_component internally which handles signature updates
            array->add_default_component(event.entity_id);
          }

          // Record undo if requested
          if (event.record_undo) {
            Engine::ECS::EntityID entity_id = event.entity_id;
            Engine::ECS::ComponentTypeID component_type_id = event.component_type_id;
            ReflectionLibrary::GenericValue component_data = event.component_data;

            Engine::UndoManager::record(
              // Do: Add component
              [entity_id, component_type_id, component_data]() {
                Engine::EventManager::send_event<Engine::Events::AddComponentEvent>(
                  entity_id, component_type_id, component_data, false
                );
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              },
              // Undo: Remove component
              [entity_id, component_type_id]() {
                Engine::EventManager::send_event<Engine::Events::RemoveComponentEvent>(
                  entity_id, component_type_id, false
                );
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              }
            );

            // Mark level as dirty
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        }
      );

      // Register remove component event (reflection-based)
      Engine::EventManager::add_event_listener<Engine::Events::RemoveComponentEvent>([](const Engine::Events::RemoveComponentEvent& event)
        {
          auto entity = Engine::ECS::get_entity(event.entity_id);
          if (!entity.is_valid()) {
            return;
          }

          // Get the component array for this component type
          auto& containers = Engine::ECS::Internal::get_containers();
          auto& comp_ptr = containers.component_arrays[event.component_type_id];
          if (!comp_ptr) {
            return;
          }

          Engine::ECS::IComponentArray* array = comp_ptr.get();

          // Capture component data before removal for undo
          ReflectionLibrary::GenericValue component_data;
          if (event.record_undo) {
            component_data = array->to_generic_value(event.entity_id);
          }

          // Remove the component
          containers.components_to_remove.emplace_back(event.entity_id, event.component_type_id);

          // Record undo if requested
          if (event.record_undo) {
            Engine::ECS::EntityID entity_id = event.entity_id;
            Engine::ECS::ComponentTypeID component_type_id = event.component_type_id;

            Engine::UndoManager::record(
              // Do: Remove component
              [entity_id, component_type_id]() {
                Engine::EventManager::send_event<Engine::Events::RemoveComponentEvent>(
                  entity_id, component_type_id, false
                );
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              },
              // Undo: Restore component
              [entity_id, component_type_id, component_data]() {
                Engine::EventManager::send_event<Engine::Events::AddComponentEvent>(
                  entity_id, component_type_id, component_data, false
                );
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              }
            );

            // Mark level as dirty
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        }
      );

      // Register entity deletion event
      Engine::EventManager::add_event_listener<Engine::Events::DeleteEntityEvent>([](const Engine::Events::DeleteEntityEvent& event)
        {
          auto entity = Engine::ECS::get_entity(event.entity_id);
          if (!entity.is_valid()) {
            return;
          }

          // Record undo if requested
          if (event.record_undo) {
            // Capture entity state before deletion
            Engine::ECS::Prefab entity_prefab = Engine::ECS::entity_to_prefab(entity);
            Engine::ECS::EntityID entity_id = event.entity_id;

            Engine::UndoManager::record(
              // Do: Delete the entity
              [entity_id]() {
                Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(entity_id, false);
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              },
              // Undo: Recreate the entity from prefab with the same ID
              [entity_prefab, entity_id]() {
                Engine::ECS::Entity restored = Engine::ECS::instantiate_prefab_with_id(entity_prefab, entity_id);
                // Send selection event to select the restored entity
                Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(restored.get_id());
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              }
            );

            // Mark level as dirty
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }

          // Perform the actual deletion
          Engine::ECS::destroy_entity(event.entity_id);
        }
      );
    }
  }
}
