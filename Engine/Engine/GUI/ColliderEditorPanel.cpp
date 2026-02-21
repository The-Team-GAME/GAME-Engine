//-----------------------------------------------------------------------------------
// file:    ColliderEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Antigravity
//
// brief:   This file implements the ColliderEditorPanel ImGui interface for visualizing
//          and editing entity colliders. It renders the entity's sprite and overlays
//          the collider AABB, allowing for visual resizing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ColliderEditorPanel.h"
#include "../Precompiled.h"
#include "widgets/ColliderEditorWidget.h"
#include "../ECS/ECS.h"
#include <algorithm>
#include "../Components/CollisionComponents.h"
#include "../Components/StateMachineComponents.h"
#include "../Components/GraphicsComponents.h"
#include "../Components/SpatialComponents.h"
#include "../Components/UIComponents.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../Graphics/Renderer.h" // For Texture
#include "../InputManager.h"
#include "../Resources.h"
#include "../Logger.h"
#include "../FileIO/ColliderMeshFileIO.h"
#include "../FileIO/FileDialog.h"

namespace {
  enum class EditorMode { None, Entity };
  enum class ButtonColliderMode { Normal, Hovered, Clicked }; // For DynamicButton editing

  EditorMode current_mode = EditorMode::None;
  ButtonColliderMode button_mode = ButtonColliderMode::Normal; // Default to normal state

  s32 selected_entity_id = -1;

  bool selection_initialized = false;
  std::string preview_state_name = "";
  float preview_animation_start_time = 0.0f;

  bool is_open = false;
  std::string selected_state_name = ""; // Moved from local static to namespace for OpenForState access

  float zoom_level = 1.0f;
  ImVec2 pan_offset = { 0.0f, 0.0f };

  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        selected_entity_id = static_cast<s32>(event.entity_id);
        if (selected_entity_id != -1) {
          current_mode = EditorMode::Entity;
        }
      }
    );
  }
}

namespace ColliderEditor {
  bool IsOpen() { return is_open; }
  void Close() { is_open = false; }

  void Open(Engine::ECS::Entity entity) {
    current_mode = EditorMode::Entity;
    selected_entity_id = static_cast<s32>(entity.get_id());
    selected_state_name = "";
    is_open = true;
  }

  void OpenForState(Engine::ECS::Entity entity, const std::string& state_name) {
    current_mode = EditorMode::Entity;
    selected_entity_id = static_cast<s32>(entity.get_id());
    selected_state_name = state_name;
    is_open = true;
  }
}

void ColliderEditorPanel()
{
  init_selection_listener();

  if (!is_open) return;

  ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Collider Editor", &is_open)) {
    ImGui::End();
    return;
  }

  if (current_mode == EditorMode::None) {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
    ImGui::End();
    return;
  }

  // --- Context Retrieval ---
  Components::BoxCollider* collider = nullptr;
  Components::Texture* texture = nullptr;
  Components::StateMachine* state_machine = nullptr;
  Components::TextureAtlas* texture_atlas = nullptr;
  Components::Animation* animation = nullptr;
  Components::Transform2D* transform = nullptr;
  Components::Flip* flip = nullptr;
  Components::DynamicButton* dynamic_button = nullptr;

  Engine::ECS::Entity entity; // Only valid in Entity mode

  if (current_mode == EditorMode::Entity) {
    if (selected_entity_id == -1) {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
      ImGui::End();
      return;
    }
    entity = Engine::ECS::get_entity(selected_entity_id);
    if (!entity.is_valid()) {
      selected_entity_id = -1;
      current_mode = EditorMode::None;
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected entity no longer exists");
      ImGui::End();
      return;
    }

    collider = entity.get_component<Components::BoxCollider>();
    texture = entity.get_component<Components::Texture>();
    state_machine = entity.get_component<Components::StateMachine>();
    texture_atlas = entity.get_component<Components::TextureAtlas>();
    animation = entity.get_component<Components::Animation>();
    transform = entity.get_component<Components::Transform2D>();
    flip = entity.get_component<Components::Flip>();
    dynamic_button = entity.get_component<Components::DynamicButton>();

    // Display DynamicButton mode selector if present
    if (dynamic_button) {
      ImGui::Text("Dynamic Button Detected");

      // Tab bar for selecting button state
      if (ImGui::BeginTabBar("ButtonStateTabBar")) {
        if (ImGui::BeginTabItem("Normal State")) {
          button_mode = ButtonColliderMode::Normal;
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hovered State")) {
          button_mode = ButtonColliderMode::Hovered;
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Clicked State")) {
          button_mode = ButtonColliderMode::Clicked;
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
      ImGui::Separator();
    }

  }

  if (!collider) {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Selected object has no BoxCollider");
    if (ImGui::Button("Add BoxCollider")) {
      entity.add_component<Components::BoxCollider>(50.0f, 50.0f, false);
    }
    ImGui::End();
    return;
  }

  // --- State Selection Logic ---
  std::string current_editing_state = "";
  bool editing_specific_state = false;

  if (state_machine) {
    ImGui::Text("State Machine Detected");

    // Dropdown to select state
    if (ImGui::BeginCombo("Edit State", selected_state_name.empty() ? "Current (Live/Default)" : selected_state_name.c_str())) {

      if (ImGui::Selectable("Current (Live/Default)", selected_state_name.empty())) {
        selected_state_name = "";
      }

      for (auto& [name, state] : state_machine->states) {
        bool is_selected = (selected_state_name == name);
        if (ImGui::Selectable(name.c_str(), is_selected)) {
          selected_state_name = name;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    if (!selected_state_name.empty()) {
      if (state_machine->states.find(selected_state_name) != state_machine->states.end()) {
        editing_specific_state = true;
        current_editing_state = selected_state_name;
      }
      else {
        // State might have been deleted
        selected_state_name = "";
      }
    }
  }

  // --- Collider Type Selection ---
  ImGui::Separator();
  ImGui::Text("Collider Type");

  // We'll read the current type and allow changing it
  static int current_type_index = 0; // 0=AABB, 1=Polygon
  static bool type_just_changed = false;
  const char* type_names[] = { "AABB", "Polygon" };

  if (ImGui::Combo("##ColliderType", &current_type_index, type_names, IM_ARRAYSIZE(type_names))) {
    // Type changed via combo box
    type_just_changed = true;
  }

  // --- Data Retrieval ---
  Engine::Graphics::Texture texture_handle = nullptr;
  const Components::TextureAtlas* active_atlas = texture_atlas;
  const Components::Animation* active_animation = animation;

  float col_width = 0.0f;
  float col_height = 0.0f;
  Math::Vec2D col_offset = { 0.0f, 0.0f };
  Components::ColliderType col_type = Components::ColliderType::AABB;
  std::vector<Math::Vec2D> col_vertices; // For polygon colliders
  bool should_apply_flip = false; // Only apply flip in live mode (Entity)

  if (editing_specific_state) {
    // STATE EDITING MODE - completely isolated from live entity
    auto& state = state_machine->states[current_editing_state];

    // Get texture from state animation if available
    if (state.animation.has_value()) {
      if (state.animation->texture.texture_handle) {
        texture_handle = state.animation->texture.texture_handle;
        active_atlas = &state.animation->texture_atlas;
        active_animation = &state.animation->animation;

        // Track preview animation start time (reset when state changes)
        if (preview_state_name != current_editing_state) {
          preview_state_name = current_editing_state;
          preview_animation_start_time = Engine::Time::get_time();
        }
      }

      // Get collider data from state if available
      // Note: Will set defaults after sprite_world_width/height are calculated
      if (state.animation->collider_data.has_value()) {
        col_type = state.animation->collider_data->type;
        col_width = state.animation->collider_data->width;
        col_height = state.animation->collider_data->height;
        col_offset = state.animation->collider_data->offset;
        col_vertices = state.animation->collider_data->vertices;
      }
    }
    else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "State has no animation data.");
    }
  }
  else {
    // LIVE/DEFAULT EDITING MODE
    if (texture && texture->texture_handle) {
      texture_handle = texture->texture_handle;
    }

    // Load from DynamicButton if present, otherwise from BoxCollider
    if (dynamic_button) {
      col_type = collider->type; // Type is always from BoxCollider
      col_offset = collider->offset; // Offset is always from BoxCollider

      if (button_mode == ButtonColliderMode::Normal) {
        col_width = dynamic_button->normal.width;
        col_height = dynamic_button->normal.height;
        col_vertices = dynamic_button->normal.vertices;
      }
      else if (button_mode == ButtonColliderMode::Hovered) {
        col_width = dynamic_button->hovered.width;
        col_height = dynamic_button->hovered.height;
        col_vertices = dynamic_button->hovered.vertices;
      }
      else { // Clicked
        col_width = dynamic_button->clicked.width;
        col_height = dynamic_button->clicked.height;
        col_vertices = dynamic_button->clicked.vertices;
      }
    }
    else {
      col_type = collider->type;
      col_width = collider->width;
      col_height = collider->height;
      col_offset = collider->offset;
      col_vertices = collider->polygon.vertices;
    }

    should_apply_flip = true;
  }

  if (!texture_handle) {
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "No Texture to visualize");
    // Show numeric controls at least
    if (editing_specific_state) {
      auto& state = state_machine->states[current_editing_state];
      if (state.animation.has_value() && state.animation->collider_data.has_value()) {
        ImGui::DragFloat("Width", &state.animation->collider_data->width);
        ImGui::DragFloat("Height", &state.animation->collider_data->height);
        ImGui::DragFloat2("Offset", &state.animation->collider_data->offset.x);

        if (ImGui::Button("Remove Override")) {
          state.animation->collider_data.reset();
        }
      }
    }
    else {
      ImGui::DragFloat("Width", &collider->width);
      ImGui::DragFloat("Height", &collider->height);
      ImGui::DragFloat2("Offset", &collider->offset.x);
    }
    ImGui::End();
    return;
  }

  // --- Visualization Logic ---

  // Calculate UVs
  ImVec2 uv0 = { 0, 0 };
  ImVec2 uv1 = { 1, 1 };

  // Determine Sprite World Dimensions (Virtual Pixels)
  float sprite_world_width = 0.0f;
  float sprite_world_height = 0.0f;

  // 1. Base Scale from Transform2D or DynamicButton state
  if (dynamic_button) {
    // Use transform from the selected button state
    const Components::DynamicButton::StateData* current_state = nullptr;
    if (button_mode == ButtonColliderMode::Normal) {
      current_state = &dynamic_button->normal;
    }
    else if (button_mode == ButtonColliderMode::Hovered) {
      current_state = &dynamic_button->hovered;
    }
    else {
      current_state = &dynamic_button->clicked;
    }

    if (current_state) {
      sprite_world_width = current_state->transform.scale.x;
      sprite_world_height = current_state->transform.scale.y;
    }
  }
  else if (transform) {
    sprite_world_width = transform->scale.x;
    sprite_world_height = transform->scale.y;
  }
  else if (texture_handle) {
    // Fallback if no transform (unlikely for rendered entity)
    sprite_world_width = static_cast<float>(texture_handle->width);
    sprite_world_height = static_cast<float>(texture_handle->height);
  }

  // 2. State Override (only when editing specific state)
  if (editing_specific_state) {
    auto& state = state_machine->states[current_editing_state];
    if (state.animation.has_value() && state.animation->scale.has_value()) {
      sprite_world_width = state.animation->scale->x;
      sprite_world_height = state.animation->scale->y;
    }

    // Now set defaults for states without collider override
    if (!state.animation.has_value() || !state.animation->collider_data.has_value()) {
      col_width = sprite_world_width;
      col_height = sprite_world_height;
      col_offset = { 0.0f, 0.0f };

      if (state.animation.has_value()) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "No collider override for this state.");
        ImGui::Text("Default: %.0fx%.0f, offset (0, 0)", col_width, col_height);
        if (ImGui::Button("Create Override with Defaults")) {
          Components::ColliderData data;
          data.type = col_type;
          data.width = col_width;
          data.height = col_height;
          data.offset = col_offset;
          data.vertices = col_vertices;
          state.animation->collider_data = data;
        }

        // Copy From State dropdown (even when no override exists yet)
        ImGui::SameLine();
        if (ImGui::BeginCombo("##CopyFromNoOverride", "Copy From...")) {
          // Option: Live/Default collider
          if (collider && ImGui::Selectable("Live/Default")) {
            Components::ColliderData data;
            data.type = collider->type;
            data.width = collider->width;
            data.height = collider->height;
            data.offset = collider->offset;
            data.vertices = collider->polygon.vertices;
            state.animation->collider_data = data;
          }
          // Option: Other states
          for (auto& [other_name, other_state] : state_machine->states) {
            if (other_name == current_editing_state) continue;
            if (!other_state.animation.has_value() || !other_state.animation->collider_data.has_value()) continue;
            if (ImGui::Selectable(other_name.c_str())) {
              state.animation->collider_data = other_state.animation->collider_data.value();
            }
          }
          ImGui::EndCombo();
        }
      }
    }
  }

  // 3. UV Calculation (for texture subset)
  if (active_atlas && !active_atlas->subTextures.empty()) {
    int atlas_index = 0;
    if (active_animation && !active_animation->frames.empty()) {
      if (editing_specific_state) {
        // Calculate animated frame using preview start time
        // Always loop in preview mode for better visualization
        float elapsed = Engine::Time::get_time() - preview_animation_start_time;
        float time_to_use = elapsed;

        if (active_animation->totalDuration > 0.0f) {
          time_to_use = fmod(elapsed, active_animation->totalDuration);
        }

        int current_frame = 0;
        for (int i = 0; i < static_cast<int>(active_animation->frames.size()); ++i) {
          if (time_to_use >= active_animation->frames[i].before_time) {
            current_frame = i;
          }
          else {
            break;
          }
        }

        atlas_index = active_animation->frames[current_frame].atlas_index;
      }
      else {
        // Use live animation's current frame (Entity mode only)
        atlas_index = active_animation->get_current_atlas_index();
      }
    }
    else {
      // No animation, use drawIndex if valid, else 0
      if (active_atlas->drawIndex >= 0) {
        atlas_index = active_atlas->drawIndex;
      }
    }

    if (atlas_index >= 0 && atlas_index < static_cast<int>(active_atlas->subTextures.size())) {
      const auto& subTex = active_atlas->subTextures[atlas_index];
      // tex_coords: 0=BL, 1=BR, 2=TR, 3=TL
      // ImGui wants TL (uv0) and BR (uv1)
      uv0 = { subTex.tex_coords[3].x, subTex.tex_coords[3].y };
      uv1 = { subTex.tex_coords[1].x, subTex.tex_coords[1].y };
    }
  }

  // Setup sprite rendering configuration
  ImGuiWidgets::SpriteRenderConfig sprite_config;
  sprite_config.texture_handle = texture_handle;
  sprite_config.uv0 = uv0;
  sprite_config.uv1 = uv1;
  sprite_config.sprite_world_width = sprite_world_width;
  sprite_config.sprite_world_height = sprite_world_height;

  // --- Contained Visual Editor Area ---
  ImVec2 avail = ImGui::GetContentRegionAvail();
  // Reserve space for controls below the visual area
  float child_height = avail.y - 180.0f;
  if (child_height < 200.0f) child_height = 200.0f;
  ImVec2 child_size = { avail.x, child_height };

  ImGui::BeginChild("##ColliderEditorCanvas", child_size, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  // Handle zoom and pan inside the child region
  bool canvas_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);
  if (canvas_hovered) {
    float scroll = ImGui::GetIO().MouseWheel;
    if (scroll != 0.0f) {
      float old_zoom = zoom_level;
      zoom_level *= (scroll > 0.0f) ? 1.1f : (1.0f / 1.1f);
      zoom_level = std::max(0.1f, std::min(zoom_level, 20.0f));

      // Zoom toward mouse position
      ImVec2 mouse_pos = ImGui::GetMousePos();
      ImVec2 canvas_min = ImGui::GetWindowPos();
      ImVec2 canvas_center = { canvas_min.x + child_size.x * 0.5f + pan_offset.x,
                               canvas_min.y + child_size.y * 0.5f + pan_offset.y };
      float zoom_ratio = zoom_level / old_zoom;
      pan_offset.x = mouse_pos.x - (mouse_pos.x - canvas_center.x) * zoom_ratio - (canvas_min.x + child_size.x * 0.5f);
      pan_offset.y = mouse_pos.y - (mouse_pos.y - canvas_center.y) * zoom_ratio - (canvas_min.y + child_size.y * 0.5f);
    }

    // Middle-mouse drag to pan
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      pan_offset.x += delta.x;
      pan_offset.y += delta.y;
    }
  }

  // Calculate visual editor layout using the child region size
  ImGuiWidgets::VisualEditorLayout layout = ImGuiWidgets::CalculateVisualEditorLayout(sprite_config, child_size);

  // Apply zoom and pan to layout
  layout.scale *= zoom_level;
  layout.draw_width = sprite_config.sprite_world_width * layout.scale;
  layout.draw_height = sprite_config.sprite_world_height * layout.scale;

  // Recalculate positions with zoom and pan
  ImVec2 canvas_pos = ImGui::GetWindowPos();
  float offset_x = (child_size.x - layout.draw_width) * 0.5f + pan_offset.x;
  float offset_y = (child_size.y - layout.draw_height) * 0.5f + pan_offset.y;
  layout.image_min = { canvas_pos.x + offset_x, canvas_pos.y + offset_y };
  layout.image_max = { layout.image_min.x + layout.draw_width, layout.image_min.y + layout.draw_height };
  layout.center = { layout.image_min.x + layout.draw_width * 0.5f, layout.image_min.y + layout.draw_height * 0.5f };

  // Push clip rect to contain everything within the child window
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 clip_min = canvas_pos;
  ImVec2 clip_max = { canvas_pos.x + child_size.x, canvas_pos.y + child_size.y };
  draw_list->PushClipRect(clip_min, clip_max, true);

  // Draw background grid for visual reference
  draw_list->AddRectFilled(clip_min, clip_max, IM_COL32(30, 30, 30, 255));

  // Draw the sprite
  ImGuiWidgets::DrawSprite(sprite_config, layout);

  // Apply flip to offset ONLY in live mode
  Math::Vec2D display_offset = col_offset;
  float flip_x = 1.0f;
  float flip_y = 1.0f;
  if (should_apply_flip && flip) {
    flip_x = flip->value.x;
    flip_y = flip->value.y;
    display_offset.x *= flip_x;
    display_offset.y *= flip_y;
  }

  bool changed = false;

  // Sync current_type_index with loaded collider type only when entity/state changes
  // Don't overwrite user's combo box selection
  if (!type_just_changed) {
    current_type_index = (col_type == Components::ColliderType::POLYGON) ? 1 : 0;
  }
  type_just_changed = false;

  if (current_type_index == 0) {
    // ===== AABB MODE =====
    ImGuiWidgets::AABBColliderData aabb_data;
    aabb_data.width = col_width;
    aabb_data.height = col_height;
    aabb_data.offset = col_offset;

    changed = ImGuiWidgets::AABBColliderEditor(
      draw_list,
      layout.center,
      layout.scale,
      aabb_data,
      should_apply_flip,
      flip_x,
      flip_y
    );

    if (changed) {
      col_width = aabb_data.width;
      col_height = aabb_data.height;
      col_offset = aabb_data.offset;
    }
  }
  else {
    // ===== POLYGON MODE =====
    ImGuiWidgets::PolygonColliderData poly_data;
    poly_data.vertices = col_vertices;

    changed = ImGuiWidgets::PolygonColliderEditor(
      draw_list,
      layout.center,
      layout.scale,
      poly_data,
      display_offset,
      layout.image_min,
      layout.image_max
    );

    if (changed) {
      col_vertices = poly_data.vertices;
    }
  }

  // Apply changes
  // Also handle type changes from dropdown
  Components::ColliderType new_type = (current_type_index == 1) ? Components::ColliderType::POLYGON : Components::ColliderType::AABB;
  if (new_type != col_type) {
    col_type = new_type;
    changed = true;

    // When switching to polygon, initialize with a box if no vertices
    if (col_type == Components::ColliderType::POLYGON && col_vertices.empty()) {
      float hw = col_width / 2.0f;
      float hh = col_height / 2.0f;
      col_vertices = {
        Math::Vec2D(-hw, -hh),
        Math::Vec2D(-hw, hh),
        Math::Vec2D(hw, hh),
        Math::Vec2D(hw, -hh)
      };
    }
  }

  if (changed) {
    if (editing_specific_state) {
      auto& state = state_machine->states[current_editing_state];
      if (state.animation.has_value()) {
        // Ensure collider data exists
        if (!state.animation->collider_data.has_value()) {
          state.animation->collider_data = Components::ColliderData();
          state.animation->collider_data->type = col_type;
          state.animation->collider_data->width = col_width;
          state.animation->collider_data->height = col_height;
          state.animation->collider_data->offset = col_offset;
          state.animation->collider_data->vertices = col_vertices;
        }
        else {
          state.animation->collider_data->type = col_type;
          state.animation->collider_data->width = col_width;
          state.animation->collider_data->height = col_height;
          state.animation->collider_data->offset = col_offset;
          state.animation->collider_data->vertices = col_vertices;
        }
      }
    }
    else {
      // Live/Default update
      collider->type = col_type;
      collider->offset = col_offset;

      // Save to DynamicButton if present
      if (dynamic_button) {
        if (button_mode == ButtonColliderMode::Normal) {
          dynamic_button->normal.width = col_width;
          dynamic_button->normal.height = col_height;
          dynamic_button->normal.vertices = col_vertices;
        }
        else if (button_mode == ButtonColliderMode::Hovered) {
          dynamic_button->hovered.width = col_width;
          dynamic_button->hovered.height = col_height;
          dynamic_button->hovered.vertices = col_vertices;
        }
        else { // Clicked
          dynamic_button->clicked.width = col_width;
          dynamic_button->clicked.height = col_height;
          dynamic_button->clicked.vertices = col_vertices;
        }

        // Also update BoxCollider with current values
        // (DynamicButtonSystem will override these at runtime based on hover state)
        collider->width = col_width;
        collider->height = col_height;
        collider->aabb.half_width = col_width / 2.0f;
        collider->aabb.half_height = col_height / 2.0f;

        if (col_type == Components::ColliderType::POLYGON) {
          collider->polygon.vertices = col_vertices;
        }
      }
      else {
        // No DynamicButton, save directly to BoxCollider
        collider->width = col_width;
        collider->height = col_height;
        collider->aabb.half_width = col_width / 2.0f;
        collider->aabb.half_height = col_height / 2.0f;

        if (col_type == Components::ColliderType::POLYGON) {
          collider->polygon.vertices = col_vertices;
        }
      }

    }
  }

  // Pop clip rect and end the contained canvas area
  draw_list->PopClipRect();
  ImGui::EndChild();

  // Zoom controls below the canvas
  ImGui::Text("Zoom: %.0f%%", zoom_level * 100.0f);
  ImGui::SameLine();
  if (ImGui::Button("Reset View")) {
    zoom_level = 1.0f;
    pan_offset = { 0.0f, 0.0f };
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(Scroll to zoom, Middle-drag to pan)");

  // Display values
  ImGui::Text("Collider Size: %.2f x %.2f", col_width, col_height);
  ImGui::Text("Collider Offset: %.2f, %.2f", col_offset.x, col_offset.y);

  // Transform controls for DynamicButton states
  if (dynamic_button) {
    ImGui::Separator();
    ImGui::Text("Transform Controls (per state):");

    Components::DynamicButton::StateData* current_state = nullptr;
    if (button_mode == ButtonColliderMode::Normal) {
      current_state = &dynamic_button->normal;
    }
    else if (button_mode == ButtonColliderMode::Hovered) {
      current_state = &dynamic_button->hovered;
    }
    else {
      current_state = &dynamic_button->clicked;
    }

    if (current_state) {
      bool transform_changed = false;
      transform_changed |= ImGui::DragFloat2("Position", &current_state->transform.position.x, 0.1f);
      transform_changed |= ImGui::DragFloat("Rotation", &current_state->transform.rotation, 0.5f);
      transform_changed |= ImGui::DragFloat2("Scale", &current_state->transform.scale.x, 0.01f);

      // Easing function inputs
      ImGui::Separator();
      ImGui::Text("Easing Functions:");

      // Get all registered easing functions
      static auto easing_names = Engine::FunctionRegistry::instance().get_all_names<f32(f32, f32, f32, f32)>();

      // Helper lambda for easing function combo
      auto easing_combo = [&](const char* label, Engine::SerializableFunction& easing_func) {
        bool easing_changed = false;
        if (ImGui::BeginCombo(label, easing_func.empty() ? "None (Linear)" : easing_func.name.c_str())) {
          if (ImGui::Selectable("None (Linear)", easing_func.empty())) {
            easing_func = "";
            easing_changed = true;
          }

          for (const auto& name : easing_names) {
            bool is_selected = (easing_func.name == name);
            if (ImGui::Selectable(name.c_str(), is_selected)) {
              easing_func = name;
              easing_changed = true;
            }
            if (is_selected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        return easing_changed;
        };

      transform_changed |= easing_combo("Position Easing", current_state->position_easing);
      transform_changed |= easing_combo("Rotation Easing", current_state->rotation_easing);
      transform_changed |= easing_combo("Scale Easing", current_state->scale_easing);

      if (transform_changed) {
        changed = true;
      }
    }
  }



  ImGui::SameLine();
  if (ImGui::Button("Reset to Sprite Size")) {
    col_width = sprite_world_width;
    col_height = sprite_world_height;
    col_offset = { 0.0f, 0.0f };
    changed = true;

    // Apply reset immediately
    if (editing_specific_state) {
      auto& state = state_machine->states[current_editing_state];
      if (state.animation.has_value()) {
        Components::ColliderData data;
        data.type = col_type;
        data.width = col_width;
        data.height = col_height;
        data.offset = col_offset;
        data.vertices = col_vertices;
        state.animation->collider_data = data;
      }
    }
    else {
      collider->width = col_width;
      collider->height = col_height;
      collider->offset = col_offset;
      collider->aabb.half_width = col_width / 2.0f;
      collider->aabb.half_height = col_height / 2.0f;

    }
  }

  // Export/Import collider mesh buttons
  ImGui::Separator();
  ImGui::Text("Collider Mesh File I/O:");

  if (ImGui::Button("Export Collider...")) {
    auto save_path = Engine::FileIO::show_save_file_dialog(
      "Export Collider Mesh",
      "collider_mesh",
      "Collider Mesh (*.collider.json)\0*.collider.json\0JSON Files (*.json)\0*.json\0All Files\0*.*\0",
      ".collider.json",
      Engine::Resources::resource_path
    );

    if (save_path.has_value()) {
      Engine::FileIO::ColliderMeshData mesh_data;
      mesh_data.vertices = col_vertices;
      mesh_data.offset = col_offset;
      mesh_data.width = col_width;
      mesh_data.height = col_height;

      if (Engine::FileIO::serialize_collider_mesh_to_json_file(save_path.value(), mesh_data)) {
        Log::info("Exported collider mesh to: {}", save_path.value());
      }
    }
  }

  ImGui::SameLine();

  if (ImGui::Button("Import Collider...")) {
    auto open_path = Engine::FileIO::show_open_file_dialog(
      "Import Collider Mesh",
      "Collider Mesh (*.collider.json)\0*.collider.json\0JSON Files (*.json)\0*.json\0All Files\0*.*\0",
      Engine::Resources::resource_path
    );

    if (open_path.has_value()) {
      auto mesh_data = Engine::FileIO::deserialize_collider_mesh_from_json_file(open_path.value());

      if (mesh_data.has_value()) {
        col_vertices = mesh_data->vertices;
        col_offset = mesh_data->offset;
        col_width = mesh_data->width;
        col_height = mesh_data->height;

        // Switch to polygon mode if vertices were loaded
        if (!col_vertices.empty()) {
          col_type = Components::ColliderType::POLYGON;
          current_type_index = 1;
        }

        changed = true;

        // Apply the imported data
        if (editing_specific_state) {
          auto& state = state_machine->states[current_editing_state];
          if (state.animation.has_value()) {
            if (!state.animation->collider_data.has_value()) {
              state.animation->collider_data = Components::ColliderData();
            }
            state.animation->collider_data->type = col_type;
            state.animation->collider_data->width = col_width;
            state.animation->collider_data->height = col_height;
            state.animation->collider_data->offset = col_offset;
            state.animation->collider_data->vertices = col_vertices;
          }
        }
        else {
          collider->type = col_type;
          collider->width = col_width;
          collider->height = col_height;
          collider->offset = col_offset;
          collider->aabb.half_width = col_width / 2.0f;
          collider->aabb.half_height = col_height / 2.0f;
          collider->polygon.vertices = col_vertices;
        }

        Log::info("Imported collider mesh from: {}", open_path.value());
      }
      else {
        Log::warning("Failed to import collider mesh from: {}", open_path.value());
      }
    }
  }

  if (editing_specific_state) {
    auto& state = state_machine->states[current_editing_state];
    if (state.animation.has_value() && state.animation->collider_data.has_value()) {
      ImGui::SameLine();
      if (ImGui::Button("Remove Override")) {
        state.animation->collider_data.reset();
      }

      // Copy From State dropdown
      if (ImGui::BeginCombo("Copy From State", "Copy From...")) {
        // Option: Live/Default collider
        if (collider && ImGui::Selectable("Live/Default")) {
          Components::ColliderData data;
          data.type = collider->type;
          data.width = collider->width;
          data.height = collider->height;
          data.offset = collider->offset;
          data.vertices = collider->polygon.vertices;
          state.animation->collider_data = data;
        }
        // Option: Other states
        for (auto& [other_name, other_state] : state_machine->states) {
          if (other_name == current_editing_state) continue;
          if (!other_state.animation.has_value() || !other_state.animation->collider_data.has_value()) continue;
          if (ImGui::Selectable(other_name.c_str())) {
            state.animation->collider_data = other_state.animation->collider_data.value();
          }
        }
        ImGui::EndCombo();
      }

      // Apply to All States button
      if (ImGui::Button("Apply to All States")) {
        ImGui::OpenPopup("ConfirmApplyAll");
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Copy this state's collider to all other states that have animation data");
      }
      if (ImGui::BeginPopup("ConfirmApplyAll")) {
        ImGui::Text("Apply this collider to all other states?");
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
          for (auto& [other_name, other_state] : state_machine->states) {
            if (other_name == current_editing_state) continue;
            if (!other_state.animation.has_value()) continue;
            other_state.animation->collider_data = state.animation->collider_data.value();
          }
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
    }
  }

  ImGui::End();
}
