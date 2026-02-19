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

#include "../../Precompiled.h"
#include "ReflectionFieldWidgets.h"
#include "TextureAtlasWidget.h"
#include "../ColliderEditorPanel.h"
#include "../../UndoManager/UndoManager.h"
#include "../ResourcePicker.h"
#include "../../Resources.h"
#include "../../Components/Components.h"
#include "../../Path.h"
#include <unordered_map>
#include "../../FunctionRegistry.h"

namespace ImGuiWidgets {

  bool render_nested_component_tree(
    const std::string& component_name,
    const std::function<void()>& content_func
  ) {
    // Split component name by "::"
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = 0;

    while ((end = component_name.find("::", start)) != std::string::npos) {
      parts.push_back(component_name.substr(start, end - start));
      start = end + 2;
    }
    parts.push_back(component_name.substr(start));

    // If there's only one part (no namespaces), render directly
    if (parts.size() == 1) {
      if (ImGui::TreeNode(parts[0].c_str())) {
        content_func();
        ImGui::TreePop();
        return true;
      }
      return false;
    }

    // Render nested tree nodes
    bool all_open = true;
    for (size_t i = 0; i < parts.size(); ++i) {
      // Use unique ID for each level to avoid collisions
      ImGui::PushID((component_name + "_level_" + std::to_string(i)).c_str());

      // For all but the last part, just render the namespace
      if (i < parts.size() - 1) {
        if (!ImGui::TreeNode(parts[i].c_str())) {
          all_open = false;
          ImGui::PopID();
          break;
        }
      }
      else {
        // Last part - render the actual component name
        if (ImGui::TreeNode(parts[i].c_str())) {
          content_func();
        }
        else {
          all_open = false;
        }
      }
      ImGui::PopID();
    }

    // Pop all opened tree nodes
    if (all_open) {
      for (size_t i = 0; i < parts.size(); ++i) {
        ImGui::TreePop();
      }
    }

    return all_open;
  }

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
    ImGui::SameLine();
    if (ImGui::DragFloat2("##Position", pos, 0.1f)) {
      field_value["position"]["x"] = pos[0];
      field_value["position"]["y"] = pos[1];
      modified = true;
    }

    // Rotation
    float rotation = static_cast<float>(field_value["rotation"].getDouble());
    ImGui::Text("Rotation");
    ImGui::SameLine();
    if (ImGui::DragFloat("##Rotation", &rotation, 0.1f)) {
      field_value["rotation"] = rotation;
      modified = true;
    }

    // Scale
    float scale[2] = {
        static_cast<float>(field_value["scale"]["x"].getDouble()),
        static_cast<float>(field_value["scale"]["y"].getDouble())
    };
    ImGui::Text("Scale   ");
    ImGui::SameLine();
    if (ImGui::DragFloat2("##Scale", scale, 0.1f)) {
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

    // Edit Atlas Configuration button
    if (ImGui::Button("Edit Atlas Configuration")) {
      extern void TextureAtlasEditor_Open(Engine::ECS::Entity entity);
      TextureAtlasEditor_Open(entity);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%d tiles)", static_cast<int>(atlas->subTextures.size()));

    // Use a static state per entity ID (basic but works for most cases)
    static std::unordered_map<Engine::ECS::EntityID, ImGuiWidgets::TextureAtlasState> entity_atlas_states;

    ImGuiWidgets::TextureAtlasPreview(*texture, *atlas, 80.0f, entity_atlas_states[entity.get_id()]);
    return true;
  }

  bool render_animation_field(ReflectionLibrary::GenericValue& field_value, bool& modified, Engine::ECS::Entity entity) {
    // Animation structure:
    // - durations: array of floats
    // - before_times: array of floats (cumulative times)
    // - startTime: float
    // - totalDuration: float
    // - is_looping: bool

    if (!field_value.isObject()) return false;

    // Edit Animation Frames button (if entity is valid and has Texture + TextureAtlas)
    if (entity.is_valid() && entity.has_component<Components::Texture>() && entity.has_component<Components::TextureAtlas>()) {
      if (ImGui::Button("Edit Animation Frames")) {
        extern void ImGuiWidgets_FrameAnimationEditor_OpenForAnimation(Engine::ECS::Entity entity);
        ImGuiWidgets_FrameAnimationEditor_OpenForAnimation(entity);
      }
      ImGui::Separator();
    }

    // Get values
    float total_duration = field_value["totalDuration"].isDouble() ?
      static_cast<float>(field_value["totalDuration"].getDouble()) : 0.0f;
    bool is_looping = field_value["is_looping"].isBool() ?
      field_value["is_looping"].getBool() : true;

    if (!field_value["durations"].isArray() || !field_value["before_times"].isArray()) return false;

    size_t frame_count = field_value["durations"].size();

    // Display animation info
    ImGui::Text("Animation Info:");
    ImGui::Indent();
    ImGui::Text("Frame Count: %zu", frame_count);
    ImGui::Text("Total Duration: %.3f seconds", total_duration);
    ImGui::Unindent();

    ImGui::Separator();

    // Looping toggle
    bool new_looping = is_looping;
    if (ImGui::Checkbox("Loop Animation", &new_looping)) {
      field_value["is_looping"] = new_looping;
      modified = true;
    }

    ImGui::Separator();

    // Reset start time button
    if (ImGui::Button("Reset Animation (Set Start Time to Now)")) {
      field_value["startTime"] = Engine::Time::get_time();
      modified = true;
    }

    ImGui::Separator();

    // Frame duration editor
    if (frame_count > 0) {
      ImGui::Text("Frame Durations:");
      ImGui::Indent();

      bool durations_changed = false;
      for (size_t i = 0; i < frame_count; ++i) {
        float duration = field_value["durations"][i].isDouble() ?
          static_cast<float>(field_value["durations"][i].getDouble()) : 0.0f;

        ImGui::PushID(static_cast<int>(i));
        ImGui::Text("Frame %zu:", i);
        ImGui::SameLine();

        if (ImGui::DragFloat("##duration", &duration, 0.01f, 0.0f, 10.0f, "%.3f sec")) {
          field_value["durations"][i] = duration;
          durations_changed = true;
        }

        ImGui::PopID();
      }

      // Recalculate before_times and total_duration if durations changed
      if (durations_changed) {
        float cumulative = 0.0f;
        for (size_t i = 0; i < frame_count; ++i) {
          field_value["before_times"][i] = cumulative;
          cumulative += static_cast<float>(field_value["durations"][i].getDouble());
        }
        field_value["totalDuration"] = cumulative;
        modified = true;
      }

      ImGui::Unindent();
      ImGui::Separator();

      // Quick actions for setting all durations
      static float uniform_duration = 0.1f;
      ImGui::Text("Quick Actions:");
      ImGui::Indent();
      ImGui::DragFloat("Uniform Duration", &uniform_duration, 0.01f, 0.0f, 10.0f, "%.3f sec");
      if (ImGui::Button("Set All Frames to Uniform Duration")) {
        float cumulative = 0.0f;
        for (size_t i = 0; i < frame_count; ++i) {
          field_value["durations"][i] = uniform_duration;
          field_value["before_times"][i] = cumulative;
          cumulative += uniform_duration;
        }
        field_value["totalDuration"] = cumulative;
        modified = true;
      }
      ImGui::Unindent();
    }
    else {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No frames in animation");
    }

    return true;
  }

  // Helper to render a script selector dropdown
  void render_script_selector(const std::string& label, ReflectionLibrary::GenericValue& field_value, bool& modified) {
    if (!field_value.isString()) return;

    std::string current_script = field_value.getString();

    // Get all registered functions with the correct signature
    // Note: This assumes all state callbacks have the signature void(Engine::ECS::Entity)
    auto function_names = Engine::FunctionRegistry::instance().get_all_names<void(Engine::ECS::Entity)>();

    if (ImGui::BeginCombo(label.c_str(), current_script.empty() ? "<None>" : current_script.c_str())) {

      // Option to clear selection
      bool is_selected = current_script.empty();
      if (ImGui::Selectable("<None>", is_selected)) {
        field_value = std::string("");
        modified = true;
      }
      if (is_selected) ImGui::SetItemDefaultFocus();

      // List all scripts
      for (const auto& name : function_names) {
        is_selected = (current_script == name);
        if (ImGui::Selectable(name.c_str(), is_selected)) {
          field_value = name;
          modified = true;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }

      ImGui::EndCombo();
    }
  }

  bool render_statemachine_field(ReflectionLibrary::GenericValue& field_value, bool& modified, Engine::ECS::Entity entity) {
    // StateMachine structure:
    // - states: unordered_map<string, State> (Serialized as Array of Pairs or Object)
    // Each State has:
    // - name: string
    // - animation: optional<StateAnimation>
    // Each StateAnimation has:
    // - texture: Texture
    // - texture_atlas: TextureAtlas
    // - animation: Animation
    // - scale: optional<Vec2D>

    if (!field_value.isObject()) {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "StateMachine field is not an object");
      return true;
    };

    ReflectionLibrary::Object& field_obj = field_value.getObject();
    auto states_it = field_obj.find("states");

    // Check for valid states container (Object or Array)
    if (states_it == field_obj.end() || (!states_it->second.isObject() && !states_it->second.isArray())) {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "StateMachine has no states defined (or invalid format)");
      return true;
    }

    ReflectionLibrary::GenericValue states_map_val;

    states_map_val = ReflectionLibrary::convert_array_of_pairs_to_map(states_it->second);

    ReflectionLibrary::Object& states_map = states_map_val.getObject();
    bool local_modified = false;

    ImGui::Text("States (%zu):", states_map.size());
    ImGui::Separator();

    if (states_map.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No states defined");
      return true;
    }

    // Create a tab bar for all states
    if (ImGui::BeginTabBar("StatesTabBar", ImGuiTabBarFlags_None)) {
      // Iterate through all states in the state machine
      for (std::pair<const std::string, ReflectionLibrary::GenericValue>& state_pair : states_map) {
        const std::string& state_name = state_pair.first;
        ReflectionLibrary::GenericValue& state_value = state_pair.second;

        // Create a tab for each state
        if (ImGui::BeginTabItem(state_name.c_str())) {
          // Display state name (read-only for now, as it's the key)
          ImGui::Text("State Name: %s", state_name.c_str());
          ImGui::Separator();

          // Check if state has animation
          if (state_value.isObject()) {
            ReflectionLibrary::Object& state_obj = state_value.getObject();

            // Render Script Selectors
            ImGui::Text("State Callbacks:");
            ImGui::Indent();

            auto enter_it = state_obj.find("on_enter_callback_name");
            if (enter_it != state_obj.end()) {
              render_script_selector("On Enter", enter_it->second, local_modified);
            }

            auto update_it = state_obj.find("on_update_callback_name");
            if (update_it != state_obj.end()) {
              render_script_selector("On Update", update_it->second, local_modified);
            }

            auto exit_it = state_obj.find("on_exit_callback_name");
            if (exit_it != state_obj.end()) {
              render_script_selector("On Exit", exit_it->second, local_modified);
            }

            ImGui::Unindent();
            ImGui::Separator();

            auto anim_it = state_obj.find("animation");

            // The optional<StateAnimation> is converted directly to the StateAnimation object
            // (not wrapped in a "value" key), or to null if empty
            if (anim_it != state_obj.end() && anim_it->second.isObject()) {
              ReflectionLibrary::Object& state_anim_obj = anim_it->second.getObject();

              ImGui::Text("State Animation:");
              ImGui::Indent();

              // Edit buttons at the top
              if (entity.is_valid()) {
                // Edit Atlas Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.4f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.5f, 0.3f, 1.0f));
                std::string edit_atlas_btn_label = "Edit Atlas Configuration##" + state_name;
                if (ImGui::Button(edit_atlas_btn_label.c_str(), ImVec2(-1, 0))) {
                  extern void TextureAtlasEditor_OpenForState(Engine::ECS::Entity entity, const std::string & state_name);
                  TextureAtlasEditor_OpenForState(entity, state_name);
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("Open the Texture Atlas Editor to configure the grid layout");
                }

                // Edit Animation Frames Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
                std::string edit_btn_label = "Edit Animation Frames##" + state_name;
                if (ImGui::Button(edit_btn_label.c_str(), ImVec2(-1, 0))) {
                  extern void FrameAnimationEditor_Open(Engine::ECS::Entity entity, const std::string & state_name);
                  FrameAnimationEditor_Open(entity, state_name);
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("Open the Frame Animation Editor to select specific frames from the texture atlas");
                }
                ImGui::Spacing();
              }

              // Render Texture field
              auto tex_it = state_anim_obj.find("texture");
              if (tex_it != state_anim_obj.end()) {
                ImGui::Text("Texture:");
                ImGui::Indent();
                render_texture_field(tex_it->second, local_modified);
                ImGui::Unindent();
                ImGui::Separator();
              }

              // Render Animation field
              auto anim_field_it = state_anim_obj.find("animation");
              if (anim_field_it != state_anim_obj.end()) {
                ImGui::Text("Animation:");
                ImGui::Indent();
                render_animation_field(anim_field_it->second, local_modified, entity);
                ImGui::Unindent();
              }

              ImGui::Unindent();
            }
            else {
              ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No animation defined for this state");
            }
          }

          ImGui::EndTabItem();
        }
      }

      ImGui::EndTabBar();
    }

    // Write back changes if modified
    if (local_modified) {
      states_it->second = ReflectionLibrary::convert_map_to_array_of_pairs(states_map_val);
      modified = true;
    }

    return true;
  }

  bool render_serializable_function_field(ReflectionLibrary::GenericValue& field_value, bool& modified, const std::string& field_name) {
    // SerializableFunction has a single "name" field
    if (!field_value.isObject() || field_value.getObject().find("name") == field_value.getObject().end()) {
      return false;
    }

    // auto& name_field = field_value["name"]; // TODO: implement Generic Value ref
    auto name_field = field_value["name"];
    if (!name_field.isString()) return false;

    std::string current_function = name_field.getString();

    // Display all functions from all registries
    auto easing_names = Engine::FunctionRegistry::instance().get_all_names<f32(f32, f32, f32, f32)>();
    auto entity_names = Engine::FunctionRegistry::instance().get_all_names<void(Engine::ECS::Entity)>();
    auto two_entity_names = Engine::FunctionRegistry::instance().get_all_names<void(Engine::ECS::Entity, Engine::ECS::Entity)>();
    auto void_names = Engine::FunctionRegistry::instance().get_all_names<void()>();

    ImGui::Text("%s", field_name.c_str());
    ImGui::SameLine();
    if (ImGui::BeginCombo(("##" + field_name).c_str(), current_function.empty() ? "<None>" : current_function.c_str())) {
      bool is_selected = current_function.empty();
      if (ImGui::Selectable("<None>", is_selected)) {
        name_field = std::string("");
        modified = true;
      }
      if (is_selected) ImGui::SetItemDefaultFocus();

      // Display easing functions
      if (!easing_names.empty()) {
        ImGui::TextDisabled("--- Easing Functions (f32(f32,f32,f32,f32)) ---");
        for (const auto& name : easing_names) {
          is_selected = (current_function == name);
          if (ImGui::Selectable(name.c_str(), is_selected)) {
            name_field = name;
            modified = true;
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
      }

      // Display entity callback functions
      if (!entity_names.empty()) {
        ImGui::TextDisabled("--- Entity Callbacks (void(Entity)) ---");
        for (const auto& name : entity_names) {
          is_selected = (current_function == name);
          if (ImGui::Selectable(name.c_str(), is_selected)) {
            name_field = name;
            modified = true;
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
      }

      // Display two-entity callback functions (e.g., Interactable callbacks)
      if (!two_entity_names.empty()) {
        ImGui::TextDisabled("--- Interactable Callbacks (void(Entity, Entity)) ---");
        for (const auto& name : two_entity_names) {
          is_selected = (current_function == name);
          if (ImGui::Selectable(name.c_str(), is_selected)) {
            name_field = name;
            modified = true;
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
      }

      // Display void functions
      if (!void_names.empty()) {
        ImGui::TextDisabled("--- Void Functions (void()) ---");
        for (const auto& name : void_names) {
          is_selected = (current_function == name);
          if (ImGui::Selectable(name.c_str(), is_selected)) {
            name_field = name;
            modified = true;
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }
    return true;

    /* COMMENTED OUT: Original implementation that filtered by field name
    // Determine which signature to use based on field name
    // Common patterns: callbacks ending in "callback" use void(Entity)
    //                  easing functions use f32(f32,f32,f32,f32)
    //                  click handlers use void()

    // Check if it's an easing function
    if (field_name.find("easing") != std::string::npos) {
      auto& functions = Engine::FunctionRegistry<f32(f32, f32, f32, f32)>::get_functions();

      if (ImGui::BeginCombo(label.c_str(), current_function.empty() ? "<None>" : current_function.c_str())) {
        bool is_selected = current_function.empty();
        if (ImGui::Selectable("<None>", is_selected)) {
          name_field = std::string("");
          modified = true;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();

        for (const auto& [name, func] : functions) {
          is_selected = (current_function == name);
          if (ImGui::Selectable(name.c_str(), is_selected)) {
            name_field = name;
            modified = true;
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      return true;
    }
    // Check if it's a state callback
    else if (field_name.find("callback") != std::string::npos || field_name.find("on_enter") != std::string::npos ||
      field_name.find("on_update") != std::string::npos || field_name.find("on_exit") != std::string::npos) {
      auto& functions = Engine::FunctionRegistry<void(Engine::ECS::Entity)>::get_functions();

      if (ImGui::BeginCombo(label.c_str(), current_function.empty() ? "<None>" : current_function.c_str())) {
        bool is_selected = current_function.empty();
        if (ImGui::Selectable("<None>", is_selected)) {
          name_field = std::string("");
          modified = true;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();

        for (const auto& [name, func] : functions) {
          is_selected = (current_function == name);
          if (ImGui::Selectable(name.c_str(), is_selected)) {
            name_field = name;
            modified = true;
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      return true;
    }
    // Default to void() for click handlers
    else {
      auto& functions = Engine::FunctionRegistry<void()>::get_functions();

      if (ImGui::BeginCombo(label.c_str(), current_function.empty() ? "<None>" : current_function.c_str())) {
        bool is_selected = current_function.empty();
        if (ImGui::Selectable("<None>", is_selected)) {
          name_field = std::string("");
          modified = true;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();

        for (const auto& [name, func] : functions) {
          is_selected = (current_function == name);
          if (ImGui::Selectable(name.c_str(), is_selected)) {
            name_field = name;
            modified = true;
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      return true;
    }
    */
  }

  bool render_box_collider_field(ReflectionLibrary::GenericValue& field_value, bool& modified, Engine::ECS::Entity entity, Engine::ECS::Prefab* prefab, const std::string& prefab_path) {
    if (ImGui::Button("Open Collider Editor")) {
      if (entity.is_valid()) {
        ColliderEditor::Open(entity);
      }
      else if (prefab && prefab->is_valid()) {
        ColliderEditor::Open(*prefab, prefab_path);
      }
    }


    // Render all fields of the BoxCollider
    if (field_value.isObject()) {
      auto& fields = field_value.getObject();
      for (auto& [field_name, field_val] : fields) {
        render_field(field_name, field_val.type_hint, field_val, modified, entity, prefab, prefab_path);
      }
    }

    return true;
  }

  bool render_specialized_field(
    const std::string& display_name,
    const std::string& type_hint,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity,
    Engine::ECS::Prefab* prefab,
    const std::string& prefab_path
  ) {
    if (type_hint == "Engine::SerializableFunction") {
      return render_serializable_function_field(field_value, modified, display_name);
    }
    if (type_hint == "Components::BoxCollider") {
      return render_box_collider_field(field_value, modified, entity, prefab, prefab_path);
    }
    if (type_hint == "Components::Color") {
      return render_color_field(field_value, modified);
    }

    if (type_hint == "Components::Texture") {
      return render_texture_field(field_value, modified);
    }

    if (type_hint == "Components::Font") {
      return render_font_field(field_value, modified);
    }

    if (type_hint == "Components::Transform2D") {
      return render_transform2d_field(field_value, modified);
    }

    if (type_hint == "Components::TextureAtlas") {
      return render_texture_atlas_field(entity);
    }

    if (type_hint == "Components::Animation") {
      return render_animation_field(field_value, modified, entity);
    }

    if (type_hint == "Components::StateMachine") {
      return render_statemachine_field(field_value, modified, entity);
    }

    // Not handled by specialized renderer
    return false;
  }

  bool render_generic_field(
    const std::string& display_name,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity,
    Engine::ECS::Prefab* prefab,
    const std::string& prefab_path
  ) {

    if (field_value.isBool()) {
      bool val = field_value.getBool();
      ImGui::Text("%s", display_name.c_str());
      ImGui::SameLine();
      if (ImGui::Checkbox(("##" + display_name).c_str(), &val)) {
        field_value = val;
        modified = true;
      }
    }
    else if (field_value.isInt()) {
      int val = static_cast<int>(field_value.getInt());
      ImGui::Text("%s", display_name.c_str());
      ImGui::SameLine();
      if (ImGui::DragInt(("##" + display_name).c_str(), &val)) {
        field_value = static_cast<ReflectionLibrary::Integer>(val);
        modified = true;
      }
    }
    else if (field_value.isDouble()) {
      float val = static_cast<float>(field_value.getDouble());
      ImGui::Text("%s", display_name.c_str());
      ImGui::SameLine();
      if (ImGui::DragFloat(("##" + display_name).c_str(), &val, 0.01f)) {
        field_value = static_cast<double>(val);
        modified = true;
      }
    }
    else if (field_value.isString()) {
      char buffer[1024];
      std::string str_val = field_value.getString();
      strncpy_s(buffer, str_val.c_str(), sizeof(buffer) - 1);
      buffer[sizeof(buffer) - 1] = '\0';
      ImGui::Text("%s", display_name.c_str());
      ImGui::SameLine();
      if (ImGui::InputText(("##" + display_name).c_str(), buffer, sizeof(buffer))) {
        field_value = std::string(buffer);
        modified = true;
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

          // Render the element using the field renderer with its type hint
          render_field(element_label, array[i].type_hint, array[i], modified, entity, prefab, prefab_path);

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
    const std::string& type_hint,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity,
    Engine::ECS::Prefab* prefab,
    const std::string& prefab_path
  ) {
    ImGui::PushItemWidth(-FLT_MIN);

    // Try specialized rendering first
    bool handled_by_specialization = render_specialized_field(display_name, type_hint, field_value, modified, entity, prefab, prefab_path);

    // If not handled by specialization, check if it's an object (struct) and render fields
    if (!handled_by_specialization) {
      if (field_value.isObject()) {
        auto& fields = field_value.getObject();

        // Render nested object fields under a tree node
        if (ImGui::TreeNode(display_name.c_str())) {
          for (auto& [field_name, nested_value] : fields) {
            render_field(field_name, nested_value.type_hint, nested_value, modified, entity, prefab, prefab_path);
          }
          ImGui::TreePop();
        }
      }
      else {
        // Fall back to generic rendering for primitives, arrays, etc.
        render_generic_field(display_name, field_value, modified, entity, prefab, prefab_path);
      }
    }

    ImGui::PopItemWidth();

  }

} // namespace ImGuiWidgets
