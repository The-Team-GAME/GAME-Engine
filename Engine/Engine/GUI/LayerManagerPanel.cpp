//-----------------------------------------------------------------------------------
// file:    LayerManagerPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LayerManagerPanel ImGui interface for managing
//          ECS layers. Allows creation, activation/deactivation, and inspection
//          of layers and their entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "LayerManagerPanel.h"
#include "../ECS/ECS.h"
#include <string>
#include <vector>

void LayerManagerPanel()
{
  ImGui::Begin("Layer Manager");

  auto& containers = Engine::ECS::Internal::get_containers();

  // Section 1: Layer Statistics
  if (ImGui::CollapsingHeader("Layer Statistics", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Total Registered Layers: %zu", containers.layer_id_to_name.size());

    size_t active_count = 0;
    for (Engine::ECS::LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
      if (Engine::ECS::is_layer_active(layer)) {
        active_count++;
      }
    }
    ImGui::Text("Active Layers: %zu", active_count);
    ImGui::Text("Disabled Layers: %zu", containers.layer_id_to_name.size() - active_count);
  }

  ImGui::Separator();

  // Section 2: Layer List with Enable/Disable Controls
  if (ImGui::CollapsingHeader("Layer Control", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::BeginChild("LayerList", ImVec2(0, 300), true);

    // Header
    ImGui::Columns(4, "layerColumns");
    ImGui::Separator();
    ImGui::Text("ID"); ImGui::NextColumn();
    ImGui::Text("Name"); ImGui::NextColumn();
    ImGui::Text("Entities"); ImGui::NextColumn();
    ImGui::Text("Active"); ImGui::NextColumn();
    ImGui::Separator();

    // Display all layers
    for (Engine::ECS::LayerID layer_id = 0; layer_id < containers.layer_id_to_name.size(); ++layer_id) {
      const std::string& layer_name = containers.layer_id_to_name[layer_id];

      // Skip empty layer names
      if (layer_name.empty()) continue;

      // Layer ID
      ImGui::Text("%u", layer_id);
      ImGui::NextColumn();

      // Layer Name
      ImGui::Text("%s", layer_name.c_str());
      ImGui::NextColumn();

      // Entity Count
      size_t entity_count = 0;
      if (layer_id < containers.layer_entities.size()) {
        entity_count = containers.layer_entities[layer_id].size();
      }
      ImGui::Text("%zu", entity_count);
      ImGui::NextColumn();

      // Active Checkbox
      bool is_active = Engine::ECS::is_layer_active(layer_id);
      std::string checkbox_label = "##active_" + std::to_string(layer_id);

      if (ImGui::Checkbox(checkbox_label.c_str(), &is_active)) {
        if (is_active) {
          Engine::ECS::enable_layer(layer_id);
        }
        else {
          Engine::ECS::disable_layer(layer_id);
        }
      }

      // Right-click context menu
      std::string context_id = "##context_" + std::to_string(layer_id);
      if (ImGui::BeginPopupContextItem(context_id.c_str())) {
        ImGui::Text("Layer: %s (ID: %u)", layer_name.c_str(), layer_id);
        ImGui::Separator();

        if (is_active) {
          if (ImGui::MenuItem("Disable Layer")) {
            Engine::ECS::disable_layer(layer_id);
          }
        }
        else {
          if (ImGui::MenuItem("Enable Layer")) {
            Engine::ECS::enable_layer(layer_id);
          }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("View Entities")) {
          // Could implement entity viewer here
        }

        ImGui::EndPopup();
      }

      ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::EndChild();
  }

  ImGui::Separator();

  // Section 3: Quick Actions
  if (ImGui::CollapsingHeader("Quick Actions"))
  {
    if (ImGui::Button("Enable All Layers")) {
      for (Engine::ECS::LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
        if (!containers.layer_id_to_name[layer].empty()) {
          Engine::ECS::enable_layer(layer);
        }
      }
    }

    ImGui::SameLine();

    if (ImGui::Button("Disable All Layers")) {
      for (Engine::ECS::LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
        if (!containers.layer_id_to_name[layer].empty()) {
          Engine::ECS::disable_layer(layer);
        }
      }
    }

    ImGui::Separator();

    // Toggle specific layer types
    if (ImGui::Button("Enable Default Layer Only")) {
      for (Engine::ECS::LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
        if (!containers.layer_id_to_name[layer].empty()) {
          if (layer == Engine::ECS::DEFAULT_LAYER) {
            Engine::ECS::enable_layer(layer);
          }
          else {
            Engine::ECS::disable_layer(layer);
          }
        }
      }
    }
  }

  ImGui::Separator();

  // Section 5: Layer Details
  static int selected_layer_id = -1;

  if (ImGui::CollapsingHeader("Layer Details"))
  {
    ImGui::Text("Select a layer to view details:");

    // Layer selection combo
    if (ImGui::BeginCombo("##LayerSelect",
      selected_layer_id >= 0 && selected_layer_id < static_cast<int>(containers.layer_id_to_name.size())
      ? containers.layer_id_to_name[selected_layer_id].c_str()
      : "Select Layer..."))
    {
      for (Engine::ECS::LayerID layer_id = 0; layer_id < containers.layer_id_to_name.size(); ++layer_id) {
        const std::string& layer_name = containers.layer_id_to_name[layer_id];
        if (layer_name.empty()) continue;

        bool is_selected = (selected_layer_id == static_cast<int>(layer_id));
        std::string combo_label = layer_name + " (ID: " + std::to_string(layer_id) + ")";

        if (ImGui::Selectable(combo_label.c_str(), is_selected)) {
          selected_layer_id = static_cast<int>(layer_id);
        }

        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    // Display details for selected layer
    if (selected_layer_id >= 0 && selected_layer_id < static_cast<int>(containers.layer_id_to_name.size())) {
      ImGui::Separator();

      const std::string& layer_name = containers.layer_id_to_name[selected_layer_id];
      bool is_active = Engine::ECS::is_layer_active(selected_layer_id);

      ImGui::Text("Layer Name: %s", layer_name.c_str());
      ImGui::Text("Layer ID: %d", selected_layer_id);
      ImGui::Text("Status: %s", is_active ? "ACTIVE" : "DISABLED");

      size_t entity_count = 0;
      if (selected_layer_id < static_cast<int>(containers.layer_entities.size())) {
        entity_count = containers.layer_entities[selected_layer_id].size();
      }
      ImGui::Text("Entity Count: %zu", entity_count);

      ImGui::Separator();

      // Toggle button
      if (is_active) {
        if (ImGui::Button("Disable This Layer")) {
          Engine::ECS::disable_layer(selected_layer_id);
        }
      }
      else {
        if (ImGui::Button("Enable This Layer")) {
          Engine::ECS::enable_layer(selected_layer_id);
        }
      }

      // Entity list for this layer
      if (entity_count > 0) {
        ImGui::Separator();
        ImGui::Text("Entities on this layer:");
        ImGui::BeginChild("LayerEntityList", ImVec2(0, 150), true);

        if (selected_layer_id < static_cast<int>(containers.layer_entities.size())) {
          for (Engine::ECS::EntityID entity_id : containers.layer_entities[selected_layer_id]) {
            std::string entity_label = "Entity " + std::to_string(entity_id);
            ImGui::BulletText("%s", entity_label.c_str());
          }
        }

        ImGui::EndChild();
      }
    }
  }

  ImGui::End();
}
