//-----------------------------------------------------------------------------------
// file:    LayerRegistryPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LayerRegistryPanel ImGui interface for configuring
//          the layer collision matrix. Provides a visual grid for enabling/disabling
//          collisions between layers.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "LayerRegistryPanel.h"
#include "../ECS/ECS.h"
#include <vector>
#include <string>

void LayerRegistryPanel()
{
  ImGui::Begin("Layer Collision Registry");

  auto& containers = Engine::ECS::Internal::get_containers();

  // Get all registered layers
  std::vector<Engine::ECS::LayerID> layer_ids;
  std::vector<std::string> layer_names;

  for (Engine::ECS::LayerID layer_id = 0; layer_id < containers.layer_id_to_name.size(); ++layer_id) {
    const std::string& layer_name = containers.layer_id_to_name[layer_id];
    if (!layer_name.empty()) {
      layer_ids.push_back(layer_id);
      layer_names.push_back(layer_name);
    }
  }

  if (layer_ids.empty()) {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No layers registered yet.");
    ImGui::Text("Register layers in the Entity Editor to configure collision relationships.");
    ImGui::End();
    return;
  }

  ImGui::Text("Configure which layers should collide with each other.");
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), "Checked = Layers collide | Unchecked = Layers ignore each other");
  ImGui::Separator();

  // Quick actions section
  if (ImGui::CollapsingHeader("Quick Actions", ImGuiTreeNodeFlags_DefaultOpen))
  {
    if (ImGui::Button("Enable All Collisions")) {
      Engine::ECS::LayerRegistry::reset_to_default();
    }
    ImGui::SameLine();
    if (ImGui::Button("Disable All Collisions")) {
      Engine::ECS::LayerRegistry::clear_all();
    }
    ImGui::Separator();
  }

  // Collision Matrix Display
  if (ImGui::CollapsingHeader("Collision Matrix", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Matrix: Rows vs Columns");
    ImGui::Separator();

    // Calculate dimensions
    float cell_size = 25.0f;
    float label_width = 150.0f;

    // Header row - column labels (vertical)
    ImGui::Dummy(ImVec2(label_width, cell_size));
    for (size_t col = 0; col < layer_names.size(); ++col) {
      ImGui::SameLine();
      // Rotate text by rendering vertically
      ImGui::BeginGroup();
      ImGui::Dummy(ImVec2(cell_size, 0));
      ImGui::Text("%s", layer_names[col].c_str());
      ImGui::EndGroup();
    }

    ImGui::Separator();

    // Matrix rows
    for (size_t row = 0; row < layer_ids.size(); ++row) {
      Engine::ECS::LayerID row_layer = layer_ids[row];

      // Row label
      ImGui::Text("%-20s", layer_names[row].c_str());

      // Checkboxes for this row
      for (size_t col = 0; col < layer_ids.size(); ++col) {
        Engine::ECS::LayerID col_layer = layer_ids[col];

        ImGui::SameLine();

        // Disable checkbox for diagonal (layer with itself)
        bool is_diagonal = (row == col);
        if (is_diagonal) {
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
        }

        bool collides = Engine::ECS::LayerRegistry::should_layers_collide(row_layer, col_layer);
        std::string checkbox_id = "##" + std::to_string(row_layer) + "_" + std::to_string(col_layer);

        if (is_diagonal) {
          // Always show diagonal as checked but disabled
          bool always_true = true;
          ImGui::Checkbox(checkbox_id.c_str(), &always_true);
        }
        else {
          if (ImGui::Checkbox(checkbox_id.c_str(), &collides)) {
            if (collides) {
              Engine::ECS::LayerRegistry::enable_layer_collision(row_layer, col_layer);
            }
            else {
              Engine::ECS::LayerRegistry::disable_layer_collision(row_layer, col_layer);
            }
          }
        }

        if (is_diagonal) {
          ImGui::PopStyleVar();
        }

        // Tooltip on hover
        if (ImGui::IsItemHovered() && !is_diagonal) {
          ImGui::BeginTooltip();
          ImGui::Text("%s <-> %s", layer_names[row].c_str(), layer_names[col].c_str());
          if (collides) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Collisions ENABLED");
          }
          else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Collisions DISABLED");
          }
          ImGui::EndTooltip();
        }
      }
    }
  }

  ImGui::Separator();

  // Layer-specific controls
  if (ImGui::CollapsingHeader("Layer-Specific Settings"))
  {
    static int selected_layer_index = 0;

    ImGui::Text("Select a layer to configure:");

    // Combo box to select layer
    if (ImGui::BeginCombo("##LayerSelect", layer_names[selected_layer_index].c_str())) {
      for (size_t i = 0; i < layer_names.size(); ++i) {
        bool is_selected = (selected_layer_index == static_cast<int>(i));
        if (ImGui::Selectable(layer_names[i].c_str(), is_selected)) {
          selected_layer_index = static_cast<int>(i);
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::Separator();

    Engine::ECS::LayerID selected_layer = layer_ids[selected_layer_index];
    ImGui::Text("'%s' collides with:", layer_names[selected_layer_index].c_str());

    // List all layers this layer collides with
    for (size_t i = 0; i < layer_ids.size(); ++i) {
      Engine::ECS::LayerID other_layer = layer_ids[i];
      bool collides = Engine::ECS::LayerRegistry::should_layers_collide(selected_layer, other_layer);

      // Don't allow disabling self-collision
      if (selected_layer == other_layer) {
        bool always_true = true;
        ImGui::Checkbox((layer_names[i] + " (self)").c_str(), &always_true);
      }
      else {
        if (ImGui::Checkbox(layer_names[i].c_str(), &collides)) {
          if (collides) {
            Engine::ECS::LayerRegistry::enable_layer_collision(selected_layer, other_layer);
          }
          else {
            Engine::ECS::LayerRegistry::disable_layer_collision(selected_layer, other_layer);
          }
        }
      }
    }

    ImGui::Separator();

    // Quick actions for this layer
    if (ImGui::Button("Enable All for This Layer")) {
      for (Engine::ECS::LayerID other_layer : layer_ids) {
        Engine::ECS::LayerRegistry::enable_layer_collision(selected_layer, other_layer);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Disable All for This Layer")) {
      for (Engine::ECS::LayerID other_layer : layer_ids) {
        if (other_layer != selected_layer) { // Keep self-collision
          Engine::ECS::LayerRegistry::disable_layer_collision(selected_layer, other_layer);
        }
      }
    }
  }

  ImGui::End();
}