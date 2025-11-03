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
//          properties including position, zoom, and rotation. Provides an intuitive
//          UI with presets and real-time camera manipulation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "imgui.h"
#include "Core.h"
#include "Graphics/Renderer.h"
#include "ECS/ECS.h"
#include "Components/Components.h"
#include <vector>

inline void CameraPanel()
{
  ImGui::Begin("Camera Control");

  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Camera Entities");
  ImGui::Separator();

  // Get all entities with Camera component
  std::vector<Engine::ECS::Entity> camera_entities;
  Engine::ECS::for_each_entity_with<Components::Camera>([&](Engine::ECS::Entity entity) {
    camera_entities.push_back(entity);
    });

  // Static variable to track the selected camera entity
  static Engine::ECS::EntityID selected_camera_id = Engine::ECS::INVALID_ENTITY;

  // Display list of camera entities
  if (ImGui::CollapsingHeader("Camera List", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Total Cameras: %zu", camera_entities.size());
    ImGui::Separator();

    if (camera_entities.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No camera entities found");
      ImGui::TextWrapped("Create an entity and add a Camera component to it.");
    }
    else {
      ImGui::BeginChild("CameraList", ImVec2(0, 150), true);

      for (auto& entity : camera_entities) {
        auto* camera = entity.get_component<Components::Camera>();
        if (!camera) continue;

        bool is_selected = (selected_camera_id == entity.get_id());
        bool is_active = camera->is_active;

        std::string label = "Entity " + std::to_string(entity.get_id());
        if (is_active) label += " (ACTIVE)";

        if (ImGui::Selectable(label.c_str(), is_selected)) {
          // When a camera is selected, automatically make it active
          selected_camera_id = entity.get_id();

          // Deactivate all cameras first
          for (auto& e : camera_entities) {
            auto* cam = e.get_component<Components::Camera>();
            if (cam) cam->is_active = false;
          }
          // Activate the selected camera
          camera->is_active = true;
        }
      }

      ImGui::EndChild();
    }
  }

  ImGui::Separator();

  // If a camera is selected, show its properties for editing
  if (selected_camera_id != Engine::ECS::INVALID_ENTITY) {
    Engine::ECS::Entity selected_entity = Engine::ECS::get_entity(selected_camera_id);

    if (selected_entity.is_valid() && selected_entity.has_component<Components::Camera>()) {
      auto* camera = selected_entity.get_component<Components::Camera>();

      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Edit Camera: Entity %u", selected_camera_id);
      ImGui::Separator();

      // Get virtual resolution
      Math::Vec2D virtual_resolution = Engine::Graphics::get_virtual_resolution();

      // Active status
      ImGui::Checkbox("Active Camera", &camera->is_active);
      if (camera->is_active) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Currently Active");
      }

      ImGui::Separator();

      // Position Controls
      if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::DragFloat("X", &camera->position.x, 1.0f);
        ImGui::DragFloat("Y", &camera->position.y, 1.0f);

        if (ImGui::Button("Reset Position"))
        {
          camera->position = { 0.0f, 0.0f };
        }

        ImGui::SameLine();

        if (ImGui::Button("Center on Origin"))
        {
          camera->position = { virtual_resolution.x / 2.0f, virtual_resolution.y / 2.0f };
        }

        // Quick position presets
        if (ImGui::TreeNode("Quick Presets"))
        {
          if (ImGui::Button("Top-Left"))
          {
            camera->position = { 0.0f, 0.0f };
          }

          ImGui::SameLine();

          if (ImGui::Button("Top-Right"))
          {
            camera->position = { virtual_resolution.x, 0.0f };
          }

          if (ImGui::Button("Bottom-Left"))
          {
            camera->position = { 0.0f, virtual_resolution.y };
          }

          ImGui::SameLine();

          if (ImGui::Button("Bottom-Right"))
          {
            camera->position = { virtual_resolution.x, virtual_resolution.y };
          }

          ImGui::TreePop();
        }
      }

      ImGui::Separator();

      // Zoom Controls
      if (ImGui::CollapsingHeader("Zoom", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::SliderFloat("Zoom Level", &camera->zoom, 0.1f, 5.0f, "%.2f");

        ImGui::Text("Shortcuts:");

        if (ImGui::Button("Zoom In (+25%)"))
        {
          camera->zoom *= 1.25f;
          if (camera->zoom > 5.0f) camera->zoom = 5.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Zoom Out (-25%)"))
        {
          camera->zoom *= 0.75f;
          if (camera->zoom < 0.1f) camera->zoom = 0.1f;
        }

        if (ImGui::Button("Reset Zoom (1.0x)"))
        {
          camera->zoom = 1.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Fit Screen"))
        {
          camera->zoom = 1.0f;
        }

        // Quick zoom presets
        if (ImGui::TreeNode("Zoom Presets"))
        {
          if (ImGui::Button("0.25x"))
          {
            camera->zoom = 0.25f;
          }
          ImGui::SameLine();
          if (ImGui::Button("0.5x"))
          {
            camera->zoom = 0.5f;
          }
          ImGui::SameLine();
          if (ImGui::Button("1.0x"))
          {
            camera->zoom = 1.0f;
          }
          ImGui::SameLine();
          if (ImGui::Button("2.0x"))
          {
            camera->zoom = 2.0f;
          }
          ImGui::SameLine();
          if (ImGui::Button("4.0x"))
          {
            camera->zoom = 4.0f;
          }

          ImGui::TreePop();
        }
      }

      ImGui::Separator();

      // Rotation Controls
      if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::SliderFloat("Angle (degrees)", &camera->rotation, 0.0f, 360.0f, "%.1f°");

        // Wrap rotation to 0-360 range
        while (camera->rotation < 0.0f) camera->rotation += 360.0f;
        while (camera->rotation >= 360.0f) camera->rotation -= 360.0f;

        if (ImGui::Button("Rotate +15°"))
        {
          camera->rotation += 15.0f;
          if (camera->rotation >= 360.0f) camera->rotation -= 360.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Rotate -15°"))
        {
          camera->rotation -= 15.0f;
          if (camera->rotation < 0.0f) camera->rotation += 360.0f;
        }

        if (ImGui::Button("Rotate +90°"))
        {
          camera->rotation += 90.0f;
          if (camera->rotation >= 360.0f) camera->rotation -= 360.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Rotate -90°"))
        {
          camera->rotation -= 90.0f;
          if (camera->rotation < 0.0f) camera->rotation += 360.0f;
        }

        if (ImGui::Button("Reset Rotation (0°)"))
        {
          camera->rotation = 0.0f;
        }

        // Quick rotation presets
        if (ImGui::TreeNode("Rotation Presets"))
        {
          if (ImGui::Button("0°"))
          {
            camera->rotation = 0.0f;
          }
          ImGui::SameLine();
          if (ImGui::Button("45°"))
          {
            camera->rotation = 45.0f;
          }
          ImGui::SameLine();
          if (ImGui::Button("90°"))
          {
            camera->rotation = 90.0f;
          }
          ImGui::SameLine();
          if (ImGui::Button("180°"))
          {
            camera->rotation = 180.0f;
          }
          ImGui::SameLine();
          if (ImGui::Button("270°"))
          {
            camera->rotation = 270.0f;
          }

          ImGui::TreePop();
        }
      }

      ImGui::Separator();

      // Combined Reset
      if (ImGui::Button("Reset All Camera Settings", ImVec2(-1, 0)))
      {
        camera->position = { 0.0f, 0.0f };
        camera->rotation = 0.0f;
        camera->zoom = 1.0f;
      }

      ImGui::Separator();

      // Display info
      if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Virtual Resolution:");
        ImGui::Text("  %.0f x %.0f", virtual_resolution.x, virtual_resolution.y);

        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current Camera:");
        ImGui::Text("  Position: (%.1f, %.1f)", camera->position.x, camera->position.y);
        ImGui::Text("  Zoom: %.2fx", camera->zoom);
        ImGui::Text("  Rotation: %.1f°", camera->rotation);
        ImGui::Text("  Active: %s", camera->is_active ? "Yes" : "No");

        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Status:");
        ImGui::TextWrapped("✓ Camera entity system is active");

        ImGui::Separator();

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Tips:");
        ImGui::TextWrapped("• Select a camera from the list above to edit it");
        ImGui::TextWrapped("• Only one camera can be active at a time");
        ImGui::TextWrapped("• Use 'Set Active' to switch between cameras");
        ImGui::TextWrapped("• All changes apply in real-time to the component");
      }
    }
    else {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Selected camera no longer exists!");
      selected_camera_id = Engine::ECS::INVALID_ENTITY;
    }
  }
  else {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a camera entity from the list above to edit its properties.");
  }

  ImGui::End();
}