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
#include "imgui.h"
#include "Core.h"
#include "Graphics/Renderer.h"
#include "ECS/ECS.h"
#include "Components/Components.h"
#include "InputManager.h"
#include <vector>

inline void CameraPanel()
{
  ImGui::Begin("Camera Control");

  // Check if Ctrl is being held for free cam mode
  bool ctrl_held = Engine::Input::is_held(Engine::KeyCode::CtrlLeft) ||
    Engine::Input::is_held(Engine::KeyCode::CtrlRight);

  // Get all entities with Camera component
  std::vector<Engine::ECS::Entity> camera_entities;
  Engine::ECS::Entity active_camera(Engine::ECS::INVALID_ENTITY);
  
  Engine::ECS::for_each_entity_with<Components::Camera>([&](Engine::ECS::Entity entity) {
    camera_entities.push_back(entity);
    auto* cam = entity.get_component<Components::Camera>();
    if (cam && cam->is_active) {
      active_camera = entity;
    }
    });

  bool has_active_camera = active_camera.is_valid();
  Engine::Graphics::Camera current_renderer_camera = Engine::Graphics::get_camera();

  // === MODE INDICATOR ===
  ImGui::PushStyleColor(ImGuiCol_Text, has_active_camera ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
  ImGui::TextWrapped(has_active_camera ? "MODE: Entity Camera" : "MODE: Free Camera");
  ImGui::PopStyleColor();
  
  if (ctrl_held) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[CTRL - Camera Control Active]");
  }

  ImGui::Separator();

  // === CAMERA CONTROLS ===
  if (has_active_camera) {
    // Entity camera mode
    auto* camera = active_camera.get_component<Components::Camera>();
    
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Camera: %s", camera->name.c_str());
    ImGui::Text("Entity ID: %u", active_camera.get_id());
    ImGui::Separator();

    // Position
    ImGui::Text("Position:");
    ImGui::DragFloat("X##pos", &camera->position.x, 1.0f);
    ImGui::DragFloat("Y##pos", &camera->position.y, 1.0f);

    // Zoom
    ImGui::Separator();
    ImGui::Text("Zoom:");
    ImGui::SliderFloat("##zoom", &camera->zoom, 0.1f, 10.0f, "%.2fx");
    
    // Quick zoom buttons
    ImGui::SameLine();
    if (ImGui::SmallButton("0.5x")) camera->zoom = 0.5f;
    ImGui::SameLine();
    if (ImGui::SmallButton("1x")) camera->zoom = 1.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("2x")) camera->zoom = 2.0f;

    // Rotation
    ImGui::Separator();
    ImGui::Text("Rotation:");
    ImGui::SliderFloat("##rot", &camera->rotation, 0.0f, 360.0f, "%.1f°");

    // Quick rotation buttons
    if (ImGui::Button("0°")) camera->rotation = 0.0f;
    ImGui::SameLine();
    if (ImGui::Button("90°")) camera->rotation = 90.0f;
    ImGui::SameLine();
    if (ImGui::Button("180°")) camera->rotation = 180.0f;
    ImGui::SameLine();
    if (ImGui::Button("270°")) camera->rotation = 270.0f;

    // Reset button
    ImGui::Separator();
    if (ImGui::Button("Reset Camera", ImVec2(-1, 0))) {
      camera->position = Math::Vec2D(0.0f, 0.0f);
      camera->zoom = 1.0f;
      camera->rotation = 0.0f;
    }

    // Deactivate button
    if (ImGui::Button("Deactivate (Switch to Free Cam)", ImVec2(-1, 0))) {
      camera->is_active = false;
    }
  }
  else {
    // Free camera mode
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Free Camera Mode");
    ImGui::TextWrapped("Hold Ctrl and drag with left mouse to move camera. No entity camera is active.");
    ImGui::Separator();

    // Display current free cam state
    ImGui::Text("Position: %.1f, %.1f", current_renderer_camera.position.x, current_renderer_camera.position.y);
    ImGui::Text("Zoom: %.2fx", current_renderer_camera.zoom);
    ImGui::Text("Rotation: %.1f°", current_renderer_camera.rotation);

    ImGui::Separator();

    // Manual controls for free cam
    bool changed = false;
    Math::Vec2D new_pos = current_renderer_camera.position;
    float new_zoom = current_renderer_camera.zoom;
    float new_rot = current_renderer_camera.rotation;

    changed |= ImGui::DragFloat("X##freepos", &new_pos.x, 1.0f);
    changed |= ImGui::DragFloat("Y##freepos", &new_pos.y, 1.0f);
    changed |= ImGui::SliderFloat("Zoom##free", &new_zoom, 0.1f, 10.0f, "%.2fx");
    changed |= ImGui::SliderFloat("Rotation##free", &new_rot, 0.0f, 360.0f, "%.1f°");

    if (changed) {
      current_renderer_camera.position = new_pos;
      current_renderer_camera.zoom = new_zoom;
      current_renderer_camera.rotation = new_rot;
      Engine::Graphics::set_camera(current_renderer_camera);
    }

    // Quick controls
    if (ImGui::Button("Reset Free Cam", ImVec2(-1, 0))) {
      Engine::Graphics::reset_camera();
    }
  }

  ImGui::Separator();

  // === CAMERA ENTITY LIST ===
  if (ImGui::CollapsingHeader("Camera Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Total Camera Entities: %zu", camera_entities.size());
    
    if (camera_entities.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No camera entities in scene");
    }
    else {
      ImGui::BeginChild("CameraEntityList", ImVec2(0, 120), true);
      
      for (auto& entity : camera_entities) {
        auto* cam = entity.get_component<Components::Camera>();
        if (!cam) continue;

        ImGui::PushID(entity.get_id());
        
        bool is_this_active = cam->is_active;
        std::string label = cam->name + " [ID:" + std::to_string(entity.get_id()) + "]";
        
        if (is_this_active) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
          ImGui::Text("[ACTIVE] %s", label.c_str());
          ImGui::PopStyleColor();
        }
        else {
          ImGui::Text("%s", label.c_str());
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        if (ImGui::SmallButton(is_this_active ? "Deactivate" : "Activate")) {
          // Deactivate all other cameras
          for (auto& other : camera_entities) {
            auto* other_cam = other.get_component<Components::Camera>();
            if (other_cam) other_cam->is_active = false;
          }
          // Toggle this camera
          cam->is_active = !is_this_active;
        }

        ImGui::PopID();
      }
      
      ImGui::EndChild();
    }

    // Button to deactivate all cameras (enter free cam)
    if (has_active_camera) {
      if (ImGui::Button("Deactivate All (Free Cam)", ImVec2(-1, 0))) {
        for (auto& entity : camera_entities) {
          auto* cam = entity.get_component<Components::Camera>();
          if (cam) cam->is_active = false;
        }
      }
    }
  }

  // === INSTRUCTIONS ===
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Instructions")) {
    ImGui::TextWrapped("• Hold CTRL and drag with LEFT MOUSE to pan the camera");
    ImGui::TextWrapped("• When no camera entity is active, you're in FREE CAM mode");
    ImGui::TextWrapped("• When a camera entity is active, CTRL+Drag moves that entity");
    ImGui::TextWrapped("• CTRL prevents selecting/dragging game objects");
  }

  ImGui::End();
}