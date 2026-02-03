//-----------------------------------------------------------------------------------
// file:    TextureAtlasEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the TextureAtlasEditorPanel ImGui interface.
//          Provides visual editing of TextureAtlas parameters with live preview.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Precompiled.h"
#include "TextureAtlasEditorPanel.h"
#include "../ECS/ECS.h"
#include "../Components/GraphicsComponents.h"
#include "../Time.h"
#include "../Logger.h"
#include "widgets/TextureAtlasWidget.h"
#include <algorithm>

namespace {
  // Editor state
  bool is_open = false;
  s32 selected_entity_id = -1;

  // Atlas configuration state
  int config_rows = 1;
  int config_cols = 1;
  int config_tile_width = 32;
  int config_tile_height = 32;
  int config_total_subtextures = 0;  // 0 means fill entire grid

  // Preview state
  float preview_zoom = 1.0f;
  bool show_grid_overlay = true;
  bool auto_calculate_from_texture = true;

  // Cache
  Components::Texture cached_texture;
  Components::TextureAtlas preview_atlas;
  bool cache_valid = false;

  // Load current atlas configuration from entity
  void load_from_entity() {
    if (selected_entity_id < 0) {
      cache_valid = false;
      return;
    }

    Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
    if (!entity.is_valid()) {
      cache_valid = false;
      return;
    }

    auto* texture = entity.get_component<Components::Texture>();
    auto* atlas = entity.get_component<Components::TextureAtlas>();

    if (!texture || !atlas) {
      cache_valid = false;
      return;
    }

    cached_texture = *texture;

    // Try to infer grid parameters from existing atlas
    size_t subtex_count = atlas->subTextures.size();
    if (subtex_count > 0 && cached_texture.texture_handle && cached_texture.texture_handle->id != 0) {
      // Try to detect grid from first subtexture
      const auto& first_sub = atlas->subTextures[0];
      float u_width = first_sub.tex_coords[1].x - first_sub.tex_coords[0].x;
      float v_height = first_sub.tex_coords[0].y - first_sub.tex_coords[3].y;

      if (u_width > 0 && v_height > 0) {
        config_cols = static_cast<int>(std::round(1.0f / u_width));
        config_rows = static_cast<int>(std::round(1.0f / v_height));
        config_tile_width = cached_texture.texture_handle->width / config_cols;
        config_tile_height = cached_texture.texture_handle->height / config_rows;
        config_total_subtextures = static_cast<int>(subtex_count);
      }
    }

    // Generate preview atlas
    preview_atlas = Components::TextureAtlas(
      config_tile_width, config_tile_height,
      static_cast<size_t>(config_rows), static_cast<size_t>(config_cols),
      config_total_subtextures > 0 ? static_cast<size_t>(config_total_subtextures) : 0
    );

    cache_valid = true;
  }

  // Regenerate preview atlas based on current config
  void regenerate_preview() {
    preview_atlas = Components::TextureAtlas(
      config_tile_width, config_tile_height,
      static_cast<size_t>(config_rows), static_cast<size_t>(config_cols),
      config_total_subtextures > 0 ? static_cast<size_t>(config_total_subtextures) : 0
    );
  }

  // Apply configuration to entity's TextureAtlas
  void apply_to_entity() {
    if (selected_entity_id < 0) return;

    Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
    if (!entity.is_valid()) return;

    auto* atlas = entity.get_component<Components::TextureAtlas>();
    if (!atlas) return;

    *atlas = Components::TextureAtlas(
      config_tile_width, config_tile_height,
      static_cast<size_t>(config_rows), static_cast<size_t>(config_cols),
      config_total_subtextures > 0 ? static_cast<size_t>(config_total_subtextures) : 0
    );

    Log::info("Applied TextureAtlas configuration: {}x{} grid, {}x{} tile size, {} subtextures",
      config_cols, config_rows, config_tile_width, config_tile_height,
      config_total_subtextures > 0 ? config_total_subtextures : config_rows * config_cols);
  }

  // Auto-calculate tile dimensions from texture size
  void auto_calculate_tile_size() {
    if (!cached_texture.texture_handle || cached_texture.texture_handle->id == 0) return;
    if (config_cols <= 0 || config_rows <= 0) return;

    config_tile_width = cached_texture.texture_handle->width / config_cols;
    config_tile_height = cached_texture.texture_handle->height / config_rows;
  }

} // anonymous namespace

namespace TextureAtlasEditor {

  void Open(Engine::ECS::Entity entity) {
    selected_entity_id = static_cast<s32>(entity.get_id());
    is_open = true;
    cache_valid = false;
    preview_zoom = 1.0f;
    load_from_entity();
  }

  void Close() {
    is_open = false;
    selected_entity_id = -1;
    cache_valid = false;
  }

  bool IsOpen() {
    return is_open;
  }

} // namespace TextureAtlasEditor

// Wrapper function for extern declaration (inside ImGuiWidgets namespace)
namespace ImGuiWidgets {
  void TextureAtlasEditor_Open(Engine::ECS::Entity entity) {
    TextureAtlasEditor::Open(entity);
  }
}

void TextureAtlasEditorPanel() {
  if (!is_open) return;

  ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);

  std::string window_title = "Texture Atlas Editor###TextureAtlasEditor";

  if (!ImGui::Begin(window_title.c_str(), &is_open)) {
    ImGui::End();
    return;
  }

  // Validate
  if (selected_entity_id < 0) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No entity selected");
    ImGui::End();
    return;
  }

  Engine::ECS::Entity entity(static_cast<u32>(selected_entity_id));
  if (!entity.is_valid()) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Invalid entity");
    ImGui::End();
    return;
  }

  if (!entity.has_component<Components::Texture>() || !entity.has_component<Components::TextureAtlas>()) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Entity needs Texture and TextureAtlas components");
    ImGui::End();
    return;
  }

  if (!cache_valid) {
    load_from_entity();
  }

  // ============================================================================
  // Header: Apply/Revert buttons
  // ============================================================================
  if (ImGui::Button("Apply", ImVec2(80, 0))) {
    apply_to_entity();
  }
  ImGui::SameLine();
  if (ImGui::Button("Revert", ImVec2(80, 0))) {
    load_from_entity();
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Configure the texture atlas grid.\nChanges are previewed live but only applied when you click 'Apply'.");
  }

  ImGui::Separator();

  // ============================================================================
  // Left: Configuration Panel
  // ============================================================================
  float panel_height = ImGui::GetContentRegionAvail().y;

  ImGui::BeginChild("ConfigPanel", ImVec2(250, panel_height), true);
  ImGui::Text("Atlas Configuration");
  ImGui::Separator();

  bool config_changed = false;

  // Texture info
  if (cached_texture.texture_handle && cached_texture.texture_handle->id != 0) {
    ImGui::Text("Texture Size: %dx%d",
      cached_texture.texture_handle->width,
      cached_texture.texture_handle->height);
    ImGui::Separator();
  }

  // Grid configuration
  ImGui::Text("Grid Layout:");
  ImGui::Indent();

  if (ImGui::DragInt("Columns", &config_cols, 0.1f, 1, 64)) {
    config_cols = std::max(1, config_cols);
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }

  if (ImGui::DragInt("Rows", &config_rows, 0.1f, 1, 64)) {
    config_rows = std::max(1, config_rows);
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }

  ImGui::Unindent();
  ImGui::Spacing();

  // Tile size configuration
  ImGui::Text("Tile Size:");
  ImGui::Indent();

  ImGui::Checkbox("Auto from texture", &auto_calculate_from_texture);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Automatically calculate tile size from texture dimensions and grid size");
  }

  if (auto_calculate_from_texture) {
    ImGui::BeginDisabled();
  }

  if (ImGui::DragInt("Width##tile", &config_tile_width, 1.0f, 1, 4096)) {
    config_tile_width = std::max(1, config_tile_width);
    config_changed = true;
  }

  if (ImGui::DragInt("Height##tile", &config_tile_height, 1.0f, 1, 4096)) {
    config_tile_height = std::max(1, config_tile_height);
    config_changed = true;
  }

  if (auto_calculate_from_texture) {
    ImGui::EndDisabled();
  }

  ImGui::Unindent();
  ImGui::Spacing();

  // Total subtextures
  ImGui::Text("Subtexture Count:");
  ImGui::Indent();

  int max_subtextures = config_rows * config_cols;
  ImGui::Text("Max possible: %d", max_subtextures);

  if (ImGui::DragInt("Total##subtex", &config_total_subtextures, 0.1f, 0, max_subtextures)) {
    config_total_subtextures = std::clamp(config_total_subtextures, 0, max_subtextures);
    config_changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("0 = use all tiles in grid (%d)", max_subtextures);
  }

  ImGui::Unindent();
  ImGui::Spacing();
  ImGui::Separator();

  // Quick presets
  ImGui::Text("Quick Presets:");
  if (ImGui::Button("1x1", ImVec2(40, 0))) {
    config_rows = 1; config_cols = 1; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("2x2", ImVec2(40, 0))) {
    config_rows = 2; config_cols = 2; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("4x4", ImVec2(40, 0))) {
    config_rows = 4; config_cols = 4; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("8x8", ImVec2(40, 0))) {
    config_rows = 8; config_cols = 8; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }

  if (ImGui::Button("1x4", ImVec2(40, 0))) {
    config_rows = 1; config_cols = 4; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("1x8", ImVec2(40, 0))) {
    config_rows = 1; config_cols = 8; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("4x1", ImVec2(40, 0))) {
    config_rows = 4; config_cols = 1; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("8x1", ImVec2(40, 0))) {
    config_rows = 8; config_cols = 1; config_total_subtextures = 0;
    if (auto_calculate_from_texture) auto_calculate_tile_size();
    config_changed = true;
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Preview options
  ImGui::Text("Preview Options:");
  ImGui::Checkbox("Show grid overlay", &show_grid_overlay);
  ImGui::SliderFloat("Zoom", &preview_zoom, 0.25f, 4.0f, "%.2fx");

  // Regenerate preview if config changed
  if (config_changed) {
    regenerate_preview();
  }

  ImGui::EndChild();

  ImGui::SameLine();

  // ============================================================================
  // Right: Preview Panel
  // ============================================================================
  ImGui::BeginChild("PreviewPanel", ImVec2(0, panel_height), true, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::Text("Preview");
  ImGui::SameLine();
  ImGui::TextDisabled("(%d subtextures)", static_cast<int>(preview_atlas.subTextures.size()));
  ImGui::Separator();

  // Handle zoom with Ctrl+scroll
  if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f) {
      float zoom_factor = 1.0f + wheel * 0.15f;
      preview_zoom *= zoom_factor;
      preview_zoom = std::clamp(preview_zoom, 0.1f, 10.0f);
    }
  }

  if (cached_texture.texture_handle && cached_texture.texture_handle->id != 0) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 preview_pos = ImGui::GetCursorScreenPos();

    float tex_width = static_cast<float>(cached_texture.texture_handle->width) * preview_zoom;
    float tex_height = static_cast<float>(cached_texture.texture_handle->height) * preview_zoom;

    // Draw texture
    draw_list->AddImage(
      (ImTextureID)(intptr_t)cached_texture.texture_handle->id,
      preview_pos,
      ImVec2(preview_pos.x + tex_width, preview_pos.y + tex_height)
    );

    // Draw grid overlay
    if (show_grid_overlay && config_cols > 0 && config_rows > 0) {
      float cell_width = tex_width / config_cols;
      float cell_height = tex_height / config_rows;

      // Vertical lines
      for (int col = 0; col <= config_cols; ++col) {
        float x = preview_pos.x + col * cell_width;
        draw_list->AddLine(
          ImVec2(x, preview_pos.y),
          ImVec2(x, preview_pos.y + tex_height),
          IM_COL32(255, 255, 0, 180), 1.0f
        );
      }

      // Horizontal lines
      for (int row = 0; row <= config_rows; ++row) {
        float y = preview_pos.y + row * cell_height;
        draw_list->AddLine(
          ImVec2(preview_pos.x, y),
          ImVec2(preview_pos.x + tex_width, y),
          IM_COL32(255, 255, 0, 180), 1.0f
        );
      }

      // Highlight included subtextures
      int subtex_count = config_total_subtextures > 0 ? config_total_subtextures : config_rows * config_cols;
      int idx = 0;
      for (int row = 0; row < config_rows && idx < subtex_count; ++row) {
        for (int col = 0; col < config_cols && idx < subtex_count; ++col) {
          float x0 = preview_pos.x + col * cell_width;
          float y0 = preview_pos.y + row * cell_height;
          float x1 = x0 + cell_width;
          float y1 = y0 + cell_height;

          // Draw subtle highlight for included tiles
          draw_list->AddRectFilled(
            ImVec2(x0 + 2, y0 + 2),
            ImVec2(x1 - 2, y1 - 2),
            IM_COL32(0, 255, 0, 30)
          );

          // Draw index number
          char idx_str[8];
          snprintf(idx_str, sizeof(idx_str), "%d", idx);
          draw_list->AddText(ImVec2(x0 + 4, y0 + 2), IM_COL32(255, 255, 255, 200), idx_str);

          ++idx;
        }
      }

      // Grey out excluded tiles
      for (int row = 0; row < config_rows; ++row) {
        for (int col = 0; col < config_cols; ++col) {
          int tile_idx = row * config_cols + col;
          if (tile_idx >= subtex_count) {
            float x0 = preview_pos.x + col * cell_width;
            float y0 = preview_pos.y + row * cell_height;
            float x1 = x0 + cell_width;
            float y1 = y0 + cell_height;

            draw_list->AddRectFilled(
              ImVec2(x0, y0),
              ImVec2(x1, y1),
              IM_COL32(0, 0, 0, 150)
            );
          }
        }
      }
    }

    // Reserve space
    ImGui::Dummy(ImVec2(tex_width, tex_height));
  }
  else {
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No texture loaded");
  }

  ImGui::EndChild();

  ImGui::End();

  if (!is_open) {
    TextureAtlasEditor::Close();
  }
}
