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
#include "../Precompiled.h"
#include "LayerManagerPanel.h"
#include "../ECS/ECS.h"
#include "../ProjectSaveManager.h"
#include "../Logger.h"
#include <string>
#include <vector>
#include <algorithm>

void LayerManagerPanel()
{
  ImGui::Begin("Layer Manager");

  auto& containers = Engine::ECS::Internal::get_containers();

  // Layer Statistics
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

  // Layer List with Enable/Disable Controls
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


  //===================================================================================
  // Collision Matrix stuff
  //===================================================================================

  static float rotation_angle = 45.0f;
  static float char_spacing = 1.6f;

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

  // Collision Matrix Display
  if (ImGui::CollapsingHeader("Collision Matrix", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Configure which layers should collide with each other.");
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), "Checked = Layers collide | Unchecked = Layers ignore each other");
    ImGui::Separator();

    ImGui::Text("Matrix: Rows vs Columns");

    // Convert rotation angle to radians
    float rad = rotation_angle * 3.14159265f / 180.0f;
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);

    // Calculate maximum rotated text height
    float max_rotated_height = 0.0f;
    for (const auto& name : layer_names) {
      ImVec2 text_size = ImGui::CalcTextSize(name.c_str());
      float text_width = text_size.x * char_spacing; // Total width with spacing

      // Calculate the height of the rotated text using rotation matrix
      // Height = abs(width * sin(angle)) + abs(text_height * cos(angle))
      float rotated_height = fabsf(text_width * sin_a) + fabsf(text_size.y * cos_a);
      max_rotated_height = std::max(max_rotated_height, rotated_height);
    }

    // Header row - column labels (rotated)
    // Start position after the row label width
    ImVec2 header_start_pos = ImGui::GetCursorScreenPos();

    // First, render a dummy row to measure checkbox positions (invisible)
    ImGui::PushID("measure_row");
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.0f); // Make completely invisible
    ImGui::Text("%-20s", ""); // Empty label same width as row labels

    std::vector<float> checkbox_x_positions;
    for (size_t col = 0; col < layer_names.size(); ++col) {
      ImGui::SameLine();
      ImVec2 checkbox_pos = ImGui::GetCursorScreenPos();
      checkbox_x_positions.push_back(checkbox_pos.x);

      // Render invisible checkbox to advance cursor
      bool dummy = false;
      ImGui::Checkbox(("##dummy_" + std::to_string(col)).c_str(), &dummy);
    }
    ImGui::PopStyleVar(); // Restore alpha
    ImGui::PopID();

    // Now draw the actual column headers at the correct positions
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (size_t col = 0; col < layer_names.size(); ++col) {
      // Use the measured checkbox position
      ImVec2 pos;
      pos.x = checkbox_x_positions[col] + (ImGui::GetFrameHeight() * 0.25f); // Center on checkbox
      pos.y = header_start_pos.y + max_rotated_height + 5.0f; // Start from bottom

      // Rotation matrix
      cos_a = cosf(rad);
      sin_a = sinf(rad);

      // Draw rotated text with anchor at the end (last character)
      const char* text = layer_names[col].c_str();
      int text_len = static_cast<int>(strlen(text));

      for (int i = 0; i < text_len; i++) {
        char c[2] = { text[i], '\0' };
        ImVec2 char_size = ImGui::CalcTextSize(c);

        // Calculate offset from the END of the text (reverse direction)
        float offset = (text_len - 1 - i) * char_size.x * char_spacing;
        ImVec2 char_pos;
        char_pos.x = pos.x - offset * cos_a;
        char_pos.y = pos.y - offset * sin_a;

        draw_list->AddText(char_pos, ImGui::GetColorU32(ImGuiCol_Text), c);
      }
    }

    // Add spacing for the header
    ImGui::Dummy(ImVec2(0, max_rotated_height));

    ImGui::Separator();

    // Calculate the maximum layer name length for proper alignment (only once)
    static size_t max_name_length = [&layer_names]() {
      auto max_it = std::max_element(layer_names.begin(), layer_names.end(),
        [](const std::string& a, const std::string& b) {
          return a.length() < b.length();
        });

      Log::info("Max layer name length: {}", (max_it != layer_names.end() ? max_it->length() : 0));
      return (max_it != layer_names.end() ? max_it->length() : 0) + 1; // Add padding
      }();

    // Matrix rows
    for (size_t row = 0; row < layer_ids.size(); ++row) {
      Engine::ECS::LayerID row_layer = layer_ids[row];

      // Row label with dynamic width
      ImGui::Text("%-*s", static_cast<int>(max_name_length), layer_names[row].c_str());

      // Checkboxes for this row
      for (size_t col = 0; col < layer_ids.size(); ++col) {
        Engine::ECS::LayerID col_layer = layer_ids[col];

        ImGui::SameLine();

        bool collides = Engine::ECS::LayerRegistry::should_layers_collide(row_layer, col_layer);
        std::string checkbox_id = "##" + std::to_string(row_layer) + "_" + std::to_string(col_layer);

        if (ImGui::Checkbox(checkbox_id.c_str(), &collides)) {
          if (collides) {
            Engine::ECS::LayerRegistry::enable_layer_collision(row_layer, col_layer);
          }
          else {
            Engine::ECS::LayerRegistry::disable_layer_collision(row_layer, col_layer);
          }
          // Mark project settings as dirty
          Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings{});
        }



        // Tooltip on hover
        if (ImGui::IsItemHovered()) {
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

  // Quick actions 
  if (ImGui::Button("Enable All Collisions")) {
    Engine::ECS::LayerRegistry::reset_to_default();
    Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings{});
  }
  ImGui::SameLine();
  if (ImGui::Button("Disable All Collisions")) {
    Engine::ECS::LayerRegistry::clear_all();
    Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings{});
  }

  ImGui::Separator();

  // Layer-specific controls
  if (ImGui::CollapsingHeader("Layer-Specific Collision Settings"))
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

      if (ImGui::Checkbox(layer_names[i].c_str(), &collides)) {
        if (collides) {
          Engine::ECS::LayerRegistry::enable_layer_collision(selected_layer, other_layer);
        }
        else {
          Engine::ECS::LayerRegistry::disable_layer_collision(selected_layer, other_layer);
        }
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings{});
      }
    }

    ImGui::Separator();

    // Quick actions for this layer
    if (ImGui::Button("Enable All for This Layer")) {
      for (Engine::ECS::LayerID other_layer : layer_ids) {
        Engine::ECS::LayerRegistry::enable_layer_collision(selected_layer, other_layer);
      }
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings{});
    }
    ImGui::SameLine();
    if (ImGui::Button("Disable All for This Layer")) {
      for (Engine::ECS::LayerID other_layer : layer_ids) {
        Engine::ECS::LayerRegistry::disable_layer_collision(selected_layer, other_layer);
      }
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings{});
    }
  }

  if (ImGui::CollapsingHeader("Panel settings")) {
    ImGui::SliderFloat("Matrix Column Label Rotation", &rotation_angle, 0.0f, 90.0f, "%.1f°");
    ImGui::SliderFloat("Matrix Column Char Spacing", &char_spacing, 1.0f, 3.0f, "%.1f");
  }


  ImGui::End();
}
