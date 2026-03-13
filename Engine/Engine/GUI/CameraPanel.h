//-----------------------------------------------------------------------------------
// file:    CameraPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the CameraPanel ImGui interface for controlling camera
//          properties. Supports both entity-based cameras and free camera mode.
//          Free cam mode activates when no camera entity is active.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Precompiled.h"
#include "../ECS/ECS.h"
#include "../Components/GraphicsComponents.h"
#include "../Components/SpatialComponents.h"
#include "../Components/HierarchyComponents.h"
#include "../Graphics/Renderer.h"
#include "../Systems/GraphicsSystem.h"
#include <vector>

inline void CameraPanel()
{
  static s32 selected_camera_id = -1;
  static float test_shake_trauma = 0.5f;
  static char parent_search_buf[256] = "";  // Search filter for parent picker

  ImGui::Begin("Camera Control");

  // Collect all entities with Camera component
  std::vector<Engine::ECS::Entity> camera_entities;
  Engine::ECS::Entity active_camera_entity(Engine::ECS::INVALID_ENTITY);

  Engine::ECS::for_each_entity_with<Components::Camera>([&](Engine::ECS::Entity entity) {
    camera_entities.push_back(entity);
    auto* camera = entity.get_component<Components::Camera>();
    if (camera && camera->is_active) {
      active_camera_entity = entity;
    }
    });

  // Display camera count and mode
  ImGui::Text("Total Cameras: %zu", camera_entities.size());

  bool is_free_cam = !active_camera_entity.is_valid();
  if (is_free_cam) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "[FREE CAM MODE]");
  }

  ImGui::Separator();

  // Entity Cameras Section
  if (ImGui::CollapsingHeader("Cameras", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Select a camera to edit its properties");
    ImGui::Separator();

    // Camera list with selection
    ImGui::BeginChild("CameraList", ImVec2(0, 200), true);

    if (camera_entities.empty()) {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No camera entities found");
    }
    else {
      for (const auto& entity : camera_entities) {
        auto* camera = entity.get_component<Components::Camera>();
        if (!camera) continue;

        // Build entity label with entity name
        std::string entity_label = entity.get_name();
        if (entity_label.empty()) {
          entity_label = "Entity " + std::to_string(entity.get_id());
        }
        else {
          entity_label += " (ID: " + std::to_string(entity.get_id()) + ")";
        }

        // Add active indicator to label
        if (camera->is_active) {
          entity_label += " [ACTIVE]";
        }

        bool is_selected = (selected_camera_id == static_cast<s32>(entity.get_id()));

        if (ImGui::Selectable(entity_label.c_str(), is_selected)) {
          selected_camera_id = static_cast<s32>(entity.get_id());
        }
      }
    }

    ImGui::EndChild();

    // Create new camera button
    if (ImGui::Button("Create New Camera")) {
      Engine::ECS::Entity new_entity = Engine::ECS::create_entity();
      new_entity.add_component<Components::Camera>(false);
      new_entity.set_name("New Camera");
      new_entity.add_component<Components::Transform2D>(0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
      selected_camera_id = static_cast<s32>(new_entity.get_id());
    }

    ImGui::SameLine();

    // Delete selected camera
    if (selected_camera_id != -1 && ImGui::Button("Delete Selected")) {
      auto entity = Engine::ECS::get_entity(selected_camera_id);
      if (entity.is_valid()) {
        Engine::ECS::destroy_entity(entity);
        selected_camera_id = -1;
      }
    }
  }

  ImGui::Separator();

  // Camera Editor Section for Selected Camera
  if (selected_camera_id != -1)
  {
    auto entity = Engine::ECS::get_entity(selected_camera_id);

    if (!entity.is_valid() || !entity.has_component<Components::Camera>())
    {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected camera no longer exists!");
      selected_camera_id = -1;
    }
    else
    {
      auto* camera = entity.get_component<Components::Camera>();

      // Ensure Transform2D exists
      if (!entity.has_component<Components::Transform2D>()) {
        entity.add_component<Components::Transform2D>();
      }
      auto* transform = entity.get_component<Components::Transform2D>();

      ImGui::Text("Editing: Entity %d", selected_camera_id);
      ImGui::Separator();

      // Active toggle
      bool active = camera->is_active;
      if (ImGui::Checkbox("Active", &active)) {
        // If activating this camera, deactivate all others
        if (active) {
          Engine::ECS::for_each_entity_with<Components::Camera>([](Engine::ECS::Entity e) {
            auto* cam = e.get_component<Components::Camera>();
            if (cam) cam->is_active = false;
            });
        }
        camera->is_active = active;
      }

      ImGui::SameLine();
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Only one camera can be active at a time.\nActivating this will deactivate others.");
      }

      // Entity name
      ImGui::Text("Name:");
      char name_buffer[256];
      strncpy_s(name_buffer, entity.get_name().c_str(), sizeof(name_buffer) - 1);
      name_buffer[sizeof(name_buffer) - 1] = '\0';
      if (ImGui::InputText("##Name", name_buffer, sizeof(name_buffer))) {
        entity.set_name(name_buffer);
      }

      ImGui::Separator();

      // --- Follow Target (Parent) ---
      if (ImGui::CollapsingHeader("Follow Target", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TextDisabled("Camera will track this entity's position.");

        // Show current parent
        bool has_parent = entity.has_component<Components::Parent>();
        if (has_parent)
        {
          auto* par = entity.get_component<Components::Parent>();
          Engine::ECS::Entity parent_entity = Engine::ECS::get_entity(static_cast<s32>(par->parent_id));
          std::string parent_label = parent_entity.is_valid() ? parent_entity.get_name() : "(invalid)";
          if (parent_label.empty())
            parent_label = "Entity " + std::to_string(par->parent_id);
          ImGui::Text("Current: ");
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s (ID %u)", parent_label.c_str(), par->parent_id);
          if (ImGui::Button("Clear Follow Target"))
          {
            entity.remove_component<Components::Parent>();
          }
        }
        else
        {
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Current: None");
        }

        ImGui::Spacing();
        ImGui::Text("Set Follow Target:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##ParentSearch", "Search entities...", parent_search_buf, sizeof(parent_search_buf));

        // Collect all non-camera entities as candidates
        std::vector<Engine::ECS::Entity> all_entities;
        Engine::ECS::for_each_entity([&](Engine::ECS::Entity e) {
          if (e.get_id() != entity.get_id())
            all_entities.push_back(e);
        });

        std::string search_lower = parent_search_buf;
        for (auto& c : search_lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

        ImGui::BeginChild("##ParentList", ImVec2(0, 140), true);
        for (const auto& candidate : all_entities)
        {
          std::string cname = candidate.get_name();
          std::string display = cname.empty() ? "Entity " + std::to_string(candidate.get_id())
                                              : cname + " (ID " + std::to_string(candidate.get_id()) + ")";

          // Filter by search
          if (!search_lower.empty())
          {
            std::string display_lower = display;
            for (auto& c : display_lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (display_lower.find(search_lower) == std::string::npos)
              continue;
          }

          bool is_current_parent = has_parent &&
            entity.get_component<Components::Parent>()->parent_id == static_cast<Engine::ECS::EntityID>(candidate.get_id());

          if (is_current_parent)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));

          if (ImGui::Selectable(display.c_str(), is_current_parent))
          {
            if (entity.has_component<Components::Parent>())
              entity.remove_component<Components::Parent>();
            entity.add_component<Components::Parent>(static_cast<Engine::ECS::EntityID>(candidate.get_id()));
            parent_search_buf[0] = '\0';
          }

          if (is_current_parent)
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
      }

      ImGui::Separator();

      // Position controls
      ImGui::Text("Position:");
      float pos[2] = { transform->position.x, transform->position.y };
      if (ImGui::DragFloat2("##Pos", pos, 1.0f, -10000.0f, 10000.0f, "%.2f")) {
        transform->position = { pos[0], pos[1] };
      }

      // Rotation control
      ImGui::Text("Rotation:");
      float rotation = transform->rotation;
      if (ImGui::DragFloat("##Rot", &rotation, 0.5f, -360.0f, 360.0f, "%.2f°")) {
        transform->rotation = rotation;
      }

      // Zoom control (Scale)
      ImGui::Text("Zoom:");
      float zoom = transform->scale.x;
      if (ImGui::DragFloat("##Zoom", &zoom, 0.01f, 0.01f, 10.0f, "%.3f")) {
        transform->scale = { zoom, zoom }; // Uniform scale for zoom
      }

      ImGui::Separator();

      // --- Follow Offset ---
      ImGui::Text("Target Offset:");
      ImGui::SameLine();
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Static pixel offset from the follow target.\n"
          "X > 0 shifts camera right, Y > 0 shifts down.\n"
          "Use this to frame the player off-center.");
      }
      float offset[2] = { camera->target_offset.x, camera->target_offset.y };
      if (ImGui::DragFloat2("##TargetOffset", offset, 1.0f, -2000.0f, 2000.0f, "%.1f")) {
        camera->target_offset = { offset[0], offset[1] };
      }

      ImGui::Separator();

      // --- Smoothing ---
      if (ImGui::CollapsingHeader("Smoothing", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Smooth Movement", &camera->is_smooth);
        if (camera->is_smooth) {
          ImGui::DragFloat("Smooth Factor", &camera->smooth_factor, 0.1f, 0.1f, 50.0f, "%.1f");
          ImGui::DragFloat("Dir-Change Smooth", &camera->smooth_factor_direction_change, 0.1f, 0.1f, 50.0f, "%.1f");
          ImGui::DragFloat("Rotation Smooth", &camera->rotation_smooth, 0.1f, 0.1f, 50.0f, "%.1f");
          ImGui::DragFloat("Zoom Smooth", &camera->zoom_smooth, 0.1f, 0.1f, 50.0f, "%.1f");
        }
      }

      // --- Look-ahead ---
      if (ImGui::CollapsingHeader("Look-Ahead")) {
        ImGui::DragFloat("Look-Ahead Distance", &camera->look_ahead_distance, 1.0f, 0.0f, 1000.0f, "%.0f px");
        ImGui::DragFloat("Look-Ahead Smooth", &camera->look_ahead_smooth, 0.1f, 0.1f, 20.0f, "%.1f");
      }

      // --- Dead Zone ---
      if (ImGui::CollapsingHeader("Dead Zone")) {
        ImGui::DragFloat("Dead Zone Radius", &camera->dead_zone_radius, 0.5f, 0.0f, 200.0f, "%.1f px");
      }

      ImGui::Separator();

      // Quick actions
      if (ImGui::Button("Reset Camera")) {
        transform->position = { 0.0f, 0.0f };
        transform->rotation = 0.0f;
        transform->scale = { 1.0f, 1.0f };
        camera->target_offset = { 0.0f, 0.0f };
      }

      ImGui::SameLine();

      if (ImGui::Button("Focus at Origin")) {
        transform->position = { 0.0f, 0.0f };
      }

      ImGui::Separator();

      // Screen shake test controls
      ImGui::Text("Shake Test:");
      ImGui::DragFloat("Shake Trauma", &test_shake_trauma, 0.01f, 0.0f, 1.0f, "%.2f");

      if (ImGui::Button("Test Screen Shake")) {
        Engine::Systems::add_camera_shake(test_shake_trauma);
      }
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No camera selected");
  }

  ImGui::Separator();

  // Info section
  if (ImGui::CollapsingHeader("Info")) {
    ImGui::BulletText("Hold Ctrl + Left Mouse to pan camera");
    ImGui::BulletText("Only one entity camera can be active at a time");
    ImGui::BulletText("Free cam mode: Active when no camera entity is active");
    ImGui::BulletText("Zoom > 1.0 = zoomed in, < 1.0 = zoomed out");
    ImGui::BulletText("Rotation is in degrees (clockwise positive)");
  }

  ImGui::End();
}