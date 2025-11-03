//-----------------------------------------------------------------------------------
// file:    PrefabEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the PrefabEditorPanel ImGui interface for runtime
//          prefab manipulation. Uses reflection to automatically generate component
//          editor fields. Supports loading, editing, saving, and instantiating prefabs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "PrefabEditorPanel.h"
#include "ResourcePicker.h"
#include "widgets/TextureAtlasWidget.h"
#include "../Resources.h"
#include "../Components/Components.h"
#include "../Path.h"
#include "../FileIO/PrefabsFileIO.h"
#include <filesystem>

bool PrefabEditorPanelHelpers::render_specialized_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified) {
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

  if (display_name == "Components::Font") {
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

  if (display_name == "Components::TextureAtlas") {
    // For prefabs, we can't easily show the texture atlas widget without an entity
    // So we'll just show a message
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "TextureAtlas preview available after instantiation");
    return true;
  }

  // Not drawn here
  return false;
}

bool PrefabEditorPanelHelpers::render_generic_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified) {
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

void PrefabEditorPanelHelpers::render_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value, bool& modified) {
  // First try specialized rendering, then fallback to generic
  render_specialized_field(display_name, field_value, modified) || render_generic_field(display_name, field_value, modified);
}

void PrefabEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Prefab& prefab, Engine::ECS::IComponentArray* array)
{
  if (ImGui::TreeNode(component_name.c_str())) {
    auto component_data = array->to_generic_value(0); // Prefabs use entity ID 0
    bool modified = false;

    render_field(component_name, component_data, modified);

    // Write back changes if modified
    if (modified) {
      array->set_component_from_generic_value(0, component_data);
    }

    ImGui::Separator();

    std::string remove_label = "Remove##" + component_name;
    if (ImGui::Button(remove_label.c_str())) {
      // Remove component directly from prefab's component storage
      auto prefab_impl = prefab.get_impl();
      if (prefab_impl) {
        auto& storage = prefab_impl->get_component_storage();
        auto& signature = prefab_impl->get_signature();
        auto it = storage.find(array->get_type_id());
        if (it != storage.end()) {
          storage.erase(it);
          signature.reset(array->get_type_id());
        }
      }
    }

    ImGui::TreePop();
  }
}

void PrefabEditorPanel()
{
  static Engine::ECS::Prefab selected_prefab;
  static std::string selected_prefab_path;
  static bool prefab_modified = false;
  static char prefabSearchBuffer[256] = "";

  ImGui::Begin("Prefab Editor");

  // Section 1: Prefab List and Actions
  if (ImGui::CollapsingHeader("Prefabs", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // Search/Filter
    ImGui::InputText("Search", prefabSearchBuffer, 256);

    ImGui::Separator();

    // List available prefab files
    ImGui::BeginChild("PrefabList", ImVec2(0, 200), true);

    std::string prefabs_dir = Engine::Path(Engine::Resources::resource_path).join("prefabs").to_string();
    
    if (std::filesystem::exists(prefabs_dir)) {
      for (const auto& entry : std::filesystem::directory_iterator(prefabs_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
          std::string filename = entry.path().filename().string();
          std::string relative_path = "prefabs/" + filename;

          // Apply search filter
          if (strlen(prefabSearchBuffer) > 0 && filename.find(prefabSearchBuffer) == std::string::npos) {
            continue;
          }

          bool isSelected = (selected_prefab_path == relative_path);

          if (ImGui::Selectable(filename.c_str(), isSelected)) {
            // Load the prefab
            auto loaded_prefab = Engine::Resources::load_prefab(relative_path);
            if (loaded_prefab.is_valid()) {
              selected_prefab = loaded_prefab;
              selected_prefab_path = relative_path;
              prefab_modified = false;
            }
          }
        }
      }
    }

    ImGui::EndChild();

    // Create new prefab button
    if (ImGui::Button("Create New Prefab")) {
      selected_prefab = Engine::ECS::create_prefab();
      selected_prefab_path = "";
      prefab_modified = true;
    }

    ImGui::SameLine();

    // Save prefab button
    if (selected_prefab.is_valid()) {
      if (ImGui::Button("Save Prefab")) {
        if (selected_prefab_path.empty()) {
          ImGui::OpenPopup("SavePrefabAs");
        }
        else {
          // Save to existing path
          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(selected_prefab_path).to_string();
          if (Engine::FileIO::serialize_prefab_to_json_file(full_path, selected_prefab)) {
            prefab_modified = false;
          }
        }
      }

      ImGui::SameLine();

      // Save As button
      if (ImGui::Button("Save As...")) {
        ImGui::OpenPopup("SavePrefabAs");
      }

      ImGui::SameLine();

      // Instantiate prefab button
      if (ImGui::Button("Instantiate")) {
        Engine::ECS::instantiate_prefab(selected_prefab);
      }
    }

    // Save As popup
    static char save_filename[128] = "";
    if (ImGui::BeginPopupModal("SavePrefabAs", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Enter prefab filename:");
      ImGui::InputText("##filename", save_filename, sizeof(save_filename));
      ImGui::Text(".json");

      ImGui::Separator();

      if (ImGui::Button("Save", ImVec2(120, 0))) {
        if (strlen(save_filename) > 0) {
          std::string filename = std::string(save_filename);
          if (filename.find(".json") == std::string::npos) {
            filename += ".json";
          }
          
          std::string relative_path = "prefabs/" + filename;
          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(relative_path).to_string();
          
          // Create prefabs directory if it doesn't exist
          std::filesystem::create_directories(Engine::Path(Engine::Resources::resource_path).join("prefabs").to_string());
          
          if (Engine::FileIO::serialize_prefab_to_json_file(full_path, selected_prefab)) {
            selected_prefab_path = relative_path;
            prefab_modified = false;
            save_filename[0] = '\0';
            ImGui::CloseCurrentPopup();
          }
        }
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        save_filename[0] = '\0';
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    if (prefab_modified) {
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "* Unsaved changes");
    }
  }

  ImGui::Separator();

  // Section 2: Prefab Editor for Selected Prefab
  if (selected_prefab.is_valid())
  {
    if (!selected_prefab_path.empty()) {
      ImGui::Text("Editing: %s", selected_prefab_path.c_str());
    }
    else {
      ImGui::Text("Editing: <New Prefab>");
    }
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

        // Check if prefab already has this component
        bool has_component = false;
        selected_prefab.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray*) {
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

        // Disable button if prefab already has this component
        if (has_component) {
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
          ImGui::Button(button_label.c_str());
          ImGui::PopStyleVar();
        }
        else {
          if (ImGui::Button(button_label.c_str())) {
            // Add default-initialized component to prefab
            auto prefab_impl = selected_prefab.get_impl();
            if (prefab_impl) {
              auto& storage = prefab_impl->get_component_storage();
              auto& signature = prefab_impl->get_signature();
              
              // Create component storage if it doesn't exist in prefab
              if (storage.find(type_id) == storage.end()) {
                storage[type_id] = array->create_empty();
              }
              
              // Add default component to prefab's storage
              storage[type_id]->add_default_component(0); // Prefabs use entity ID 0
              signature.set(type_id);
              prefab_modified = true;
            }
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
      // Automatically iterate through ALL components the prefab has
      selected_prefab.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        if (!array->is_reflectable()) {
          return; // Skip non-reflectable components
        }

        // Render component tree
        PrefabEditorPanelHelpers::render_component_tree(type_name, selected_prefab, array);
      });
    }

    ImGui::Separator();

    // Layer Editor Section
    if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
    {
      auto& containers = Engine::ECS::Internal::get_containers();
      auto prefab_layers = selected_prefab.get_layers();

      // Get all registered layers
      ImGui::Text("Available Layers:");

      bool any_layers_registered = false;
      for (Engine::ECS::LayerID layer_id = 0; layer_id < containers.layer_id_to_name.size(); ++layer_id) {
        const std::string& layer_name = containers.layer_id_to_name[layer_id];

        // Skip empty layer names (shouldn't happen, but just in case)
        if (layer_name.empty()) continue;

        any_layers_registered = true;

        bool is_on_layer = prefab_layers.test(layer_id);

        if (ImGui::Checkbox((layer_name + " (ID: " + std::to_string(layer_id) + ")").c_str(), &is_on_layer)) {
          if (is_on_layer) {
            selected_prefab.add_to_layer(layer_name);
          }
          else {
            selected_prefab.remove_from_layer(layer_name);
          }
          prefab_modified = true;
        }
      }

      if (!any_layers_registered) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  (No layers registered)");
      }

      ImGui::Separator();

      // Add new layer section
      static char new_layer_name[128] = "";
      ImGui::Text("Register New Layer:");
      ImGui::InputText("##NewLayerName", new_layer_name, sizeof(new_layer_name));
      ImGui::SameLine();

      if (ImGui::Button("Register Layer")) {
        if (strlen(new_layer_name) > 0) {
          Engine::ECS::LayerID new_id = Engine::ECS::register_layer(new_layer_name);
          if (new_id != Engine::ECS::INVALID_LAYER) {
            // Clear the input
            new_layer_name[0] = '\0';
          }
        }
      }

      ImGui::Separator();

      // Quick actions
      if (ImGui::Button("Clear All Layers")) {
        selected_prefab.clear_layers();
        prefab_modified = true;
      }
    }

    ImGui::Separator();

    // Export to JSON
    if (ImGui::Button("Export to JSON (Clipboard)"))
    {
      auto generic_value = Engine::ECS::prefab_to_generic_value(selected_prefab);
      std::string json = ReflectionLibrary::generic_value_to_json(generic_value, true);
      ImGui::SetClipboardText(json.c_str());
      ImGui::OpenPopup("Export Success");
    }

    // Export success popup
    if (ImGui::BeginPopup("Export Success"))
    {
      ImGui::Text("Prefab JSON copied to clipboard!");
      ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Import from JSON
    if (ImGui::Button("Import from JSON (Clipboard)"))
    {
      const char* clipboard_text = ImGui::GetClipboardText();
      if (clipboard_text && strlen(clipboard_text) > 0) {
        try {
          auto generic_value_opt = ReflectionLibrary::generic_value_from_json(clipboard_text);
          if (generic_value_opt.has_value()) {
            auto imported_prefab = Engine::ECS::prefab_from_generic_value(generic_value_opt.value());
            if (imported_prefab.is_valid()) {
              selected_prefab = imported_prefab;
              selected_prefab_path = "";
              prefab_modified = true;
              ImGui::OpenPopup("Import Success");
            }
            else {
              ImGui::OpenPopup("Import Failed");
            }
          }
          else {
            ImGui::OpenPopup("Import Failed");
          }
        }
        catch (...) {
          ImGui::OpenPopup("Import Failed");
        }
      }
    }

    // Import success popup
    if (ImGui::BeginPopup("Import Success"))
    {
      ImGui::Text("Prefab imported successfully!");
      ImGui::EndPopup();
    }

    // Import failed popup
    if (ImGui::BeginPopup("Import Failed"))
    {
      ImGui::Text("Failed to import prefab from clipboard!");
      ImGui::EndPopup();
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No prefab selected");
  }

  ImGui::End();
}
