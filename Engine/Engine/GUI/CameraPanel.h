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

#include <imgui.h>
#include "../ECS/ECS.h"
#include "../Components/GraphicsComponents.h"
#include "../Graphics/Renderer.h"
#include <vector>

inline void CameraPanel()
{
  static s32 selected_camera_id = -1;

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

        // Build entity label with camera name
        std::string entity_label = camera->name;
        if (entity_label.empty() || entity_label == "Camera") {
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

      // Camera name
      ImGui::Text("Name:");
      char name_buffer[256];
      strncpy_s(name_buffer, camera->name.c_str(), sizeof(name_buffer) - 1);
      name_buffer[sizeof(name_buffer) - 1] = '\0';
      if (ImGui::InputText("##Name", name_buffer, sizeof(name_buffer))) {
        camera->name = name_buffer;
      }

      ImGui::Separator();

      // Position controls
      ImGui::Text("Position:");
      float pos[2] = { camera->position.x, camera->position.y };
      if (ImGui::DragFloat2("##Pos", pos, 1.0f, -10000.0f, 10000.0f, "%.2f")) {
        camera->position = { pos[0], pos[1] };
      }

      // Rotation control
      ImGui::Text("Rotation:");
      float rotation = camera->rotation;
      if (ImGui::DragFloat("##Rot", &rotation, 0.5f, -360.0f, 360.0f, "%.2f°")) {
        camera->rotation = rotation;
      }

      // Zoom control
      ImGui::Text("Zoom:");
      float zoom = camera->zoom;
      if (ImGui::DragFloat("##Zoom", &zoom, 0.01f, 0.01f, 10.0f, "%.3f")) {
        camera->zoom = zoom;
      }

      ImGui::Separator();

      // Quick actions
      if (ImGui::Button("Reset Camera")) {
        camera->position = { 0.0f, 0.0f };
        camera->rotation = 0.0f;
        camera->zoom = 1.0f;
      }

      ImGui::SameLine();

      if (ImGui::Button("Focus at Origin")) {
        camera->position = { 0.0f, 0.0f };
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