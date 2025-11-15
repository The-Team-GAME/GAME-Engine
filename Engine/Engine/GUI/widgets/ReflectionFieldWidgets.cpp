//-----------------------------------------------------------------------------------
// file:    ReflectionFieldWidgets.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of reusable ImGui widgets for rendering reflection-based
//          component fields. Extracted from EntityEditorPanel and PrefabEditorPanel
//          to promote code reuse.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "ReflectionFieldWidgets.h"
#include "TextureAtlasWidget.h"
#include "../../UndoManager/UndoManager.h"
#include "../ResourcePicker.h"
#include "../../Resources.h"
#include "../../Components/Components.h"
#include "../../Path.h"

namespace ImGuiWidgets {

  bool render_color_field(ReflectionLibrary::GenericValue& field_value, bool& modified) {
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

  bool render_texture_field(ReflectionLibrary::GenericValue& field_value, bool& modified) {
    // Get current texture path (stored as relative path)
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
      // Load texture using relative path with resource path appended
      auto texture = Engine::Resources::load_texture(current_path, true);
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
      // Convert current relative path to absolute for comparison in picker
      pending_texture_path = current_path.empty() ? "" : Engine::Path(Engine::Resources::resource_path).join(current_path).to_string();
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
      // Pass absolute path for comparison
      std::string absolute_current = current_path.empty() ? "" : Engine::Path(Engine::Resources::resource_path).join(current_path).to_string();
      if (ResourcePicker::texture_picker_popup("TexturePicker", pending_texture_path, absolute_current)) {
        // Convert selected absolute path back to relative path for storage
        std::string relative_path = Engine::Resources::get_relative_path(pending_texture_path);
        field_value["path"] = relative_path;
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

  bool render_font_field(ReflectionLibrary::GenericValue& field_value, bool& modified) {
    // Get current font path and size (stored as relative path)
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
      // Convert current relative path to absolute for comparison in picker
      pending_font_path = current_path.empty() ? "" : Engine::Path(Engine::Resources::resource_path).join(current_path).to_string();
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
      // Pass absolute path for comparison
      std::string absolute_current = current_path.empty() ? "" : Engine::Path(Engine::Resources::resource_path).join(current_path).to_string();
      if (ResourcePicker::font_picker_popup("FontPicker", pending_font_path, pending_font_size,
        absolute_current, current_size)) {
        // Convert selected absolute path back to relative path for storage
        std::string relative_path = Engine::Resources::get_relative_path(pending_font_path);
        field_value["path"] = relative_path;
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

bool render_transform2d_field(ReflectionLibrary::GenericValue& field_value, bool& modified) {
    // Position
    float pos[2] = {
        static_cast<float>(field_value["position"]["x"].getDouble()),
        static_cast<float>(field_value["position"]["y"].getDouble())
    };
    ImGui::Text("Position");
    if (ImGui::InputFloat2("##Position", pos)) {
        field_value["position"]["x"] = pos[0];
        field_value["position"]["y"] = pos[1];
        modified = true;
    }

    // Rotation
    float rotation = static_cast<float>(field_value["rotation"].getDouble());
    ImGui::Text("Rotation");
    if (ImGui::InputFloat("##Rotation", &rotation)) {
        field_value["rotation"] = rotation;
        modified = true;
    }

    // Scale
    float scale[2] = {
        static_cast<float>(field_value["scale"]["x"].getDouble()),
        static_cast<float>(field_value["scale"]["y"].getDouble())
    };
    ImGui::Text("Scale");
    if (ImGui::InputFloat2("##Scale", scale)) { 
        field_value["scale"]["x"] = scale[0];
        field_value["scale"]["y"] = scale[1];
        modified = true;
    }

    return true;
}

  bool render_texture_atlas_field(Engine::ECS::Entity entity) {
    // Only works with valid entities
    if (!entity.is_valid()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "TextureAtlas preview available after instantiation");
      return true;
    }

    // Try to get the texture from the entity's Texture component
    Components::Texture* texture = entity.get_component<Components::Texture>();
    Components::TextureAtlas* atlas = entity.get_component<Components::TextureAtlas>();

    if (!texture || !atlas) {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "TextureAtlas requires the Texture component.");
      return true;
    }

    ImGuiWidgets::TextureAtlasPreview(*texture, *atlas, 80.0f);
    return true;
  }

  bool render_specialized_field(
    const std::string& display_name,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity
  ) {
    if (display_name == "Components::Color") {
      return render_color_field(field_value, modified);
    }

    if (display_name == "Components::Texture") {
      return render_texture_field(field_value, modified);
    }

    if (display_name == "Components::Font") {
      return render_font_field(field_value, modified);
    }

    if (display_name == "Components::Transform2D") {
      return render_transform2d_field(field_value, modified);
    }

    if (display_name == "Components::TextureAtlas") {
      return render_texture_atlas_field(entity);
    }

    

    // Not handled by specialized renderer
    return false;
  }

  bool render_generic_field(
    const std::string& display_name,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity
  ) {
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
        render_field(display_name + "::" + nested_name, nested_value, modified, entity);
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
          render_field(element_label, array[i], modified, entity);

          ImGui::PopID();
        }

        ImGui::PopID();
        ImGui::TreePop();
      }
    }

    return true;
  }

  void render_field(
    const std::string& display_name,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity
  ) {
    // First try specialized rendering, then fallback to generic
    render_specialized_field(display_name, field_value, modified, entity) || render_generic_field(display_name, field_value, modified, entity);
  }

} // namespace ImGuiWidgets
