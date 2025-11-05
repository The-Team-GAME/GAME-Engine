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

  // Display camera count
  ImGui::Text("Total Cameras: %zu", camera_entities.size());
  ImGui::Separator();

  // Free Cam Mode Section
  bool is_free_cam = !active_camera_entity.is_valid();
  
  if (ImGui::CollapsingHeader("Free Camera Mode", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (is_free_cam) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FREE CAM ACTIVE");
      ImGui::Text("No entity camera is active. Free cam is controlling the view.");
      
      ImGui::Separator();
      
      // Get current camera state
      Engine::Graphics::Camera free_cam = Engine::Graphics::get_camera();
      
      // Position controls
      ImGui::Text("Position:");
      float pos[2] = { free_cam.position.x, free_cam.position.y };
      if (ImGui::DragFloat2("##FreeCamPos", pos, 1.0f, -10000.0f, 10000.0f, "%.2f")) {
        free_cam.position = { pos[0], pos[1] };
        Engine::Graphics::set_camera(free_cam);
      }
      
      // Rotation control
      ImGui::Text("Rotation:");
      float rotation = free_cam.rotation;
      if (ImGui::DragFloat("##FreeCamRot", &rotation, 0.5f, -360.0f, 360.0f, "%.2f°")) {
        free_cam.rotation = rotation;
        Engine::Graphics::set_camera(free_cam);
      }
      
      // Zoom control
      ImGui::Text("Zoom:");
      float zoom = free_cam.zoom;
      if (ImGui::DragFloat("##FreeCamZoom", &zoom, 0.01f, 0.01f, 10.0f, "%.3f")) {
        free_cam.zoom = zoom;
        Engine::Graphics::set_camera(free_cam);
      }
      
      // Reset button
      if (ImGui::Button("Reset Free Cam")) {
        Engine::Graphics::reset_camera();
      }
      
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Tip: Hold Ctrl + Left Mouse to pan camera");
    }
    else {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Free cam disabled (entity camera active)");
      ImGui::Text("Deactivate all entity cameras to use free cam.");
    }
  }

  ImGui::Separator();

  // Entity Cameras Section
  if (ImGui::CollapsingHeader("Entity Cameras", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (camera_entities.empty()) {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No camera entities found");
      ImGui::Text("Add a Camera component to an entity to create one.");
    }
    else {
      ImGui::BeginChild("CameraList", ImVec2(0, 0), true);
      
      for (auto& entity : camera_entities) {
        auto* camera = entity.get_component<Components::Camera>();
        if (!camera) continue;
        
        // Build entity label with camera name
        std::string entity_label = camera->name;
        if (entity_label.empty() || entity_label == "Camera") {
          entity_label = "Entity " + std::to_string(entity.get_id());
        } else {
          entity_label += " (Entity " + std::to_string(entity.get_id()) + ")";
        }
        
        // Header for this camera
        ImGui::PushID(entity.get_id());
        
        bool is_this_active = camera->is_active;
        ImVec4 header_color = is_this_active ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        ImGui::TextColored(header_color, "%s", entity_label.c_str());
        
        if (is_this_active) {
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[ACTIVE]");
        }
        
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
        char name_buffer[256];
        strncpy_s(name_buffer, camera->name.c_str(), sizeof(name_buffer) - 1);
        name_buffer[sizeof(name_buffer) - 1] = '\0';
        if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
          camera->name = name_buffer;
        }
        
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
        
        ImGui::SameLine();
        
        if (ImGui::Button("Go to Entity")) {
          // Jump to entity in entity editor
          ImGui::OpenPopup("GoToEntityNotification");
        }
        
        if (ImGui::BeginPopup("GoToEntityNotification")) {
          ImGui::Text("Select Entity %d in Entity Editor", entity.get_id());
          ImGui::EndPopup();
        }
        
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::PopID();
      }
      
      ImGui::EndChild();
    }
  }

  ImGui::Separator();
  
  // Info section
  if (ImGui::CollapsingHeader("Camera Controls Info")) {
    ImGui::BulletText("Hold Ctrl + Left Mouse to pan the active camera");
    ImGui::BulletText("Only one entity camera can be active at a time");
    ImGui::BulletText("When no camera is active, free cam mode is enabled");
    ImGui::BulletText("Zoom > 1.0 = zoomed in, < 1.0 = zoomed out");
    ImGui::BulletText("Rotation is in degrees (clockwise positive)");
  }

  ImGui::End();
}