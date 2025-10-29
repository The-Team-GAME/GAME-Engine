//-----------------------------------------------------------------------------------
// file:    EntityEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the EntityEditorPanel ImGui interface for runtime
//          entity and component manipulation. Uses reflection to automatically
//          generate component editor fields.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "EntityEditorPanel.h"
#include "ResourcePicker.h"
#include "../Resources.h"

bool EntityEditorPanelHelpers::render_specialized_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified) {
  if (display_name == "Components::Color") {
    ImVec4 colorVec(
      static_cast<float>(field_value["r"].getDouble()),
      static_cast<float>(field_value["g"].getDouble()),
      static_cast<float>(field_value["b"].getDouble()),
      static_cast<float>(field_value["a"].getDouble())
    );
    if (ImGui::ColorEdit4("Color", (float*)&colorVec)) {
      field_value["r"] = colorVec.x;
      field_value["g"] = colorVec.y;
      field_value["b"] = colorVec.z;
      field_value["a"] = colorVec.w;
      modified = true;
    }

    return true;
  }

  if (display_name == "Components::Texture") {
    // Get current texture path
    std::string current_path = field_value["path"].isString() ? field_value["path"].getString() : "";

    // Display current texture info
    ImGui::Text("Texture Path:");
    ImGui::SameLine();
    if (current_path.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "<None>");
    }
    else {
      ImGui::TextWrapped("%s", current_path.c_str());
    }

    // Preview current texture
    if (!current_path.empty()) {
      auto texture = Engine::Resources::load_texture(current_path, false);
      if (texture && texture->id != 0) {
        ImTextureID tex_id = static_cast<ImTextureID>(texture->id);
        float preview_size = 128.0f;
        float aspect = static_cast<float>(texture->width) / static_cast<float>(texture->height);
        ImVec2 size;
        if (aspect > 1.0f) {
          size = ImVec2(preview_size, preview_size / aspect);
        }
        else {
          size = ImVec2(preview_size * aspect, preview_size);
        }
        ImGui::Image(tex_id, size);
        ImGui::Text("Size: %dx%d", texture->width, texture->height);
      }
    }

    // Button to open texture picker
    static std::string pending_texture_path;
    static bool show_texture_picker = false;

    if (ImGui::Button("Select Texture...")) {
      pending_texture_path = current_path;
      show_texture_picker = true;
      ImGui::OpenPopup("TexturePicker");
    }

    // Clear texture button
    if (!current_path.empty()) {
      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
        field_value["path"] = std::string("");
        modified = true;
      }
    }

    // Show texture picker popup
    if (show_texture_picker) {
      if (ResourcePicker::texture_picker_popup("TexturePicker", pending_texture_path, current_path)) {
        field_value["path"] = pending_texture_path;
        modified = true;
        show_texture_picker = false;
      }

      // Check if popup was closed without selection
      if (!ImGui::IsPopupOpen("TexturePicker")) {
        show_texture_picker = false;
      }
    }

    return true;
  }

  if (display_name == "Components::Font") {
    // Get current font path and size
    std::string current_path = field_value["path"].isString() ? field_value["path"].getString() : "";
    int current_size = field_value["font_size"].isInt() ? static_cast<int>(field_value["font_size"].getInt()) : 0;

    // Display current font info
    ImGui::Text("Font Path:");
    ImGui::SameLine();
    if (current_path.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "<None>");
    }
    else {
      ImGui::TextWrapped("%s", current_path.c_str());
    }

    ImGui::Text("Font Size:");
    ImGui::SameLine();
    ImGui::Text("%dpx", current_size);

    // Manual font size input
    int new_size = current_size;
    if (ImGui::DragInt("Size##FontSize", &new_size, 1.0f, 1, 200)) {
      if (new_size > 0) {
        field_value["font_size"] = static_cast<ReflectionLibrary::Integer>(new_size);
        modified = true;
      }
    }

    // Button to open font picker
    static std::string pending_font_path;
    static int pending_font_size = 0;
    static bool show_font_picker = false;

    if (ImGui::Button("Select Font...")) {
      pending_font_path = current_path;
      pending_font_size = current_size;
      show_font_picker = true;
      ImGui::OpenPopup("FontPicker");
    }

    // Clear font button
    if (!current_path.empty()) {
      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
        field_value["path"] = std::string("");
        field_value["font_size"] = static_cast<ReflectionLibrary::Integer>(0);
        modified = true;
      }
    }

    // Show font picker popup
    if (show_font_picker) {
      if (ResourcePicker::font_picker_popup("FontPicker", pending_font_path, pending_font_size,
        current_path, current_size)) {
        field_value["path"] = pending_font_path;
        field_value["font_size"] = static_cast<ReflectionLibrary::Integer>(pending_font_size);
        modified = true;
        show_font_picker = false;
      }

      // Check if popup was closed without selection
      if (!ImGui::IsPopupOpen("FontPicker")) {
        show_font_picker = false;
      }
    }

    return true;
  }

  // Not drawn here
  return false;
}

bool EntityEditorPanelHelpers::render_generic_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified) {
  if (field_value.isBool()) {
    bool val = field_value.getBool();
    if (ImGui::Checkbox(display_name.c_str(), &val)) {
      field_value = val;
      modified = true;
    }
  }
  else if (field_value.isInt()) {
    int val = static_cast<int>(field_value.getInt());
    if (ImGui::DragInt(display_name.c_str(), &val)) {
      field_value = static_cast<ReflectionLibrary::Integer>(val);
      modified = true;
    }
  }
  else if (field_value.isDouble()) {
    float val = static_cast<float>(field_value.getDouble());
    if (ImGui::DragFloat(display_name.c_str(), &val, 0.01f)) {
      field_value = static_cast<double>(val);
      modified = true;
    }
  }
  else if (field_value.isString()) {
    char buffer[1024];
    std::string str_val = field_value.getString();
    strncpy_s(buffer, str_val.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    if (ImGui::InputText(display_name.c_str(), buffer, sizeof(buffer))) {
      field_value = std::string(buffer);
      modified = true;
    }
  }
  else if (field_value.isObject()) {
    auto& nested_fields = field_value.getObject();
    for (auto& [nested_name, nested_value] : nested_fields) {
      render_field(display_name + "::" + nested_name, nested_value, modified);
    }
  }
  else if (field_value.isArray()) {
    auto& array = field_value.getArray();
    std::string tree_label = display_name + " [" + std::to_string(array.size()) + " elements]";

    if (ImGui::TreeNode(tree_label.c_str())) {
      // Array controls
      ImGui::PushID(display_name.c_str());

      // Add element button
      if (ImGui::Button("Add Element")) {
        // Add a default element based on array type (if array has elements)
        if (!array.empty()) {
          // Clone the last element as template
          array.push_back(array.back());
        }
        else {
          // Add a default int element (could be made configurable)
          array.push_back(ReflectionLibrary::GenericValue(static_cast<ReflectionLibrary::Integer>(0)));
        }
        modified = true;
      }

      ImGui::SameLine();

      // Clear array button
      if (ImGui::Button("Clear All")) {
        array.clear();
        modified = true;
      }

      ImGui::Separator();

      // Render each element
      for (size_t i = 0; i < array.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));

        std::string element_label = "[" + std::to_string(i) + "]";

        // Remove button for this element
        if (ImGui::Button("Remove Element")) {
          array.erase(array.begin() + i);
          modified = true;
          ImGui::PopID();
          break; // Exit loop after erasing to avoid iterator issues
        }

        // Render the element using the generic field renderer
        render_field(element_label, array[i], modified);

        ImGui::PopID();
      }

      ImGui::PopID();
      ImGui::TreePop();
    }
  }

  // This should always return true, since we handled all generic 
  return true;
}

void EntityEditorPanelHelpers::render_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified) {

  // First try specialized rendering, then fallback to generic
  render_specialized_field(display_name, field_value, modified) || render_generic_field(display_name, field_value, modified);
}

void EntityEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Entity& entity, Engine::ECS::IComponentArray* array)
{
  if (ImGui::TreeNode(component_name.c_str())) {
    auto component_data = array->to_generic_value(entity.get_id());
    bool modified = false;

    render_field(component_name, component_data, modified);

    // Write back changes if modified
    if (modified) {
      //std::cout << "Component " << component_name << " modified on Entity " << entity.get_id() << std::endl;
      array->set_component_from_generic_value(entity.get_id(), component_data);
    }

    ImGui::Separator();

    std::string remove_label = "Remove##" + component_name;
    if (ImGui::Button(remove_label.c_str())) {
      auto& containers = Engine::ECS::Internal::get_containers();
      Engine::ECS::EntityID entityId = entity.get_id();

      containers.components_to_remove.emplace_back(entityId, array->get_type_id());
    }

    ImGui::TreePop();
  }
}

void EntityEditorPanel()
{
  static s32 selected_entity_id = -1;
  static char entitySearchBuffer[256] = "";

  ImGui::Begin("Entity Editor");

  // Section 1: Entity List
  if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Total Entities: %zu", Engine::ECS::get_entity_count());

    // Search/Filter
    ImGui::InputText("Search", entitySearchBuffer, 256);

    ImGui::Separator();

    // Entity list with selection
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);

    auto allEntities = Engine::ECS::get_all_entities();
    for (const auto& entity : allEntities)
    {
      bool isSelected = (selected_entity_id == static_cast<int>(entity.get_id()));
      std::string label = "Entity " + std::to_string(entity.get_id());

      if (ImGui::Selectable(label.c_str(), isSelected))
      {
        selected_entity_id = static_cast<int>(entity.get_id());
      }
    }

    ImGui::EndChild();

    // Create new entity button
    if (ImGui::Button("Create New Entity"))
    {
      auto newEntity = Engine::ECS::create_entity();
      selected_entity_id = static_cast<int>(newEntity.get_id());
    }

    ImGui::SameLine();

    // Delete selected entity
    if (selected_entity_id != -1 && ImGui::Button("Delete Selected"))
    {
      Engine::ECS::destroy_entity(selected_entity_id);
      selected_entity_id = -1;
    }
  }

  ImGui::Separator();

  // Section 2: Component Editor for Selected Entity
  if (selected_entity_id != -1)
  {
    auto entity = Engine::ECS::get_entity(selected_entity_id);

    if (!entity.is_valid())
    {
      selected_entity_id = -1;
      ImGui::End();
      return;
    }

    ImGui::Text("Editing Entity: %d", selected_entity_id);
    ImGui::Separator();

    // Add Component Section
    if (ImGui::CollapsingHeader("Add Component"))
    {
      // Get all registered component types from the ECS
      auto& containers = Engine::ECS::Internal::get_containers();

      int button_count = 0;
      for (auto& [type_id, array] : containers.component_arrays) {
        std::string component_name = array->get_type_name();

        // Skip if component type name is unknown or not reflectable
        if (component_name == "Unknown" || !array->is_reflectable()) {
          continue;
        }

        // Check if entity already has this component
        bool has_component = false;
        entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray*) {
          if (type_name == component_name) {
            has_component = true;
          }
          });

        // Create add button
        std::string button_label = "Add " + component_name;

        // Add spacing between buttons (2 per row)
        if (button_count > 0 && button_count % 2 == 0) {
          // New row
        }
        else if (button_count > 0) {
          ImGui::SameLine();
        }

        // Disable button if entity already has this component
        if (has_component) {
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
          ImGui::Button(button_label.c_str());
          ImGui::PopStyleVar();
        }
        else {
          if (ImGui::Button(button_label.c_str())) {
            // Add default-initialized component
            array->add_default_component(entity.get_id());
          }
        }

        button_count++;
      }

      if (button_count == 0) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
          "No reflectable components available");
      }
    }

    ImGui::Separator();

    // Edit Components Section
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // Automatically iterate through ALL components the entity has
      entity.for_each_component([&entity](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        if (!array->is_reflectable()) {
          return; // Skip non-reflectable components
        }

        // First try specialized rendering
        // If not handled, fallback to generic reflection-based rendering
        EntityEditorPanelHelpers::render_component_tree(type_name, entity, array);
        });
    }

    ImGui::Separator();

    // Clone Entity
    if (ImGui::Button("Clone This Entity"))
    {
      auto clonedEntity = Engine::ECS::clone_entity(entity);
      selected_entity_id = static_cast<int>(clonedEntity.get_id());
    }

    ImGui::SameLine();

    // Serialize/Export Entity
    if (ImGui::Button("Export to JSON"))
    {
      auto generic_value = Engine::ECS::entity_to_generic_value(entity);
      std::string json = ReflectionLibrary::generic_value_to_json(generic_value, true);
      ImGui::SetClipboardText(json.c_str());
      ImGui::OpenPopup("Export Success");
    }

    // Export success popup
    if (ImGui::BeginPopup("Export Success"))
    {
      ImGui::Text("Entity JSON copied to clipboard!");
      ImGui::EndPopup();
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
  }

  ImGui::End();
}

