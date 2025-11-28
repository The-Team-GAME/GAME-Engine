//-----------------------------------------------------------------------------------
// file:    TextureAtlasWidget.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of reusable ImGui widget for texture atlas preview.
//          Supports click-drag to select single or multiple tiles.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "TextureAtlasWidget.h"
#include <algorithm>

namespace ImGuiWidgets {

  void TextureAtlasPreview(
    const Components::Texture& texture,
    Components::TextureAtlas& atlas,
    float preview_height,
    TextureAtlasState& state
  ) {
    // Validate inputs
    if (texture.texture_handle->id == 0) {
      ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No texture provided!");
      return;
    }

    if (atlas.subTextures.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No subtextures defined!");
      return;
    }

    // Calculate display size while maintaining aspect ratio
    float aspect_ratio = static_cast<float>(texture.texture_handle->width) / static_cast<float>(texture.texture_handle->height);
    float display_width = preview_height * aspect_ratio;
    ImVec2 image_size(display_width, preview_height);

    // Calculate tiles per row by examining UV coordinates
    int tiles_per_row = 0;
    if (!atlas.subTextures.empty()) {
      float first_tile_x = atlas.subTextures[0].tex_coords[0].x;
      for (size_t i = 1; i < atlas.subTextures.size(); ++i) {
        if (std::abs(atlas.subTextures[i].tex_coords[0].x - first_tile_x) < 0.001f) {
          tiles_per_row = static_cast<int>(i);
          break;
        }
      }
      if (tiles_per_row == 0) tiles_per_row = static_cast<int>(atlas.subTextures.size()); // Single row
    }

    // Calculate UV bounds for each subtexture
    std::vector<std::array<float, 4>> uv_bounds; // x0, y0, x1, y1 for each subtexture
    uv_bounds.reserve(atlas.subTextures.size());

    for (const auto& sub : atlas.subTextures) {
      const auto& coords = sub.tex_coords;

      float min_x = coords[0].x, max_x = coords[0].x;
      float min_y = coords[0].y, max_y = coords[0].y;

      for (size_t j = 1; j < 4; ++j) {
        min_x = std::min(min_x, coords[j].x);
        max_x = std::max(max_x, coords[j].x);
        min_y = std::min(min_y, coords[j].y);
        max_y = std::max(max_y, coords[j].y);
      }

      uv_bounds.push_back({ min_x, min_y, max_x, max_y });
    }

    // Get cursor position before drawing image
    ImVec2 image_pos = ImGui::GetCursorScreenPos();

    // Draw the texture
    ImGui::Image(
      (ImTextureID)(intptr_t)texture.texture_handle->id,
      image_size,
      ImVec2(0, 0),
      ImVec2(1, 1),
      ImVec4(1, 1, 1, 1),
      ImVec4(0, 0, 0, 0)
    );

    // Helper lambda to find tile index under cursor
    auto get_hovered_index = [&]() -> int {
      ImVec2 mouse_pos = ImGui::GetMousePos();
      float rel_x = (mouse_pos.x - image_pos.x) / display_width;
      float rel_y = (mouse_pos.y - image_pos.y) / preview_height;

      for (int i = 0; i < static_cast<int>(uv_bounds.size()); ++i) {
        if (rel_x >= uv_bounds[i][0] && rel_x <= uv_bounds[i][2] &&
          rel_y >= uv_bounds[i][1] && rel_y <= uv_bounds[i][3]) {
          return i;
        }
      }
      return -1;
      };

    // Helper lambda to calculate rectangular selection
    auto calculate_selection = [&](int start_idx, int end_idx) -> TileSelection {
      TileSelection selection;

      if (start_idx < 0 || end_idx < 0 || tiles_per_row == 0) {
        return selection;
      }

      // Convert indices to grid coordinates
      int start_row = start_idx / tiles_per_row;
      int start_col = start_idx % tiles_per_row;
      int end_row = end_idx / tiles_per_row;
      int end_col = end_idx % tiles_per_row;

      // Get bounds
      int min_row = std::min(start_row, end_row);
      int max_row = std::max(start_row, end_row);
      int min_col = std::min(start_col, end_col);
      int max_col = std::max(start_col, end_col);

      // Select all tiles in rectangle
      for (int row = min_row; row <= max_row; ++row) {
        for (int col = min_col; col <= max_col; ++col) {
          int idx = row * tiles_per_row + col;
          if (idx < static_cast<int>(atlas.subTextures.size())) {
            selection.indices.push_back(idx);
          }
        }
      }

      std::sort(selection.indices.begin(), selection.indices.end());
      selection.grid_width = max_col - min_col + 1;
      selection.grid_height = max_row - min_row + 1;
      selection.min_row = min_row;
      selection.min_col = min_col;

      return selection;
      };

    // Handle mouse interaction
    if (ImGui::IsItemHovered()) {
      int hovered_index = get_hovered_index();

      if (hovered_index >= 0) {
        // Start dragging on click
        if (ImGui::IsMouseClicked(0)) {
          state.is_dragging = true;
          state.drag_start_index = hovered_index;
          state.current_selection = calculate_selection(hovered_index, hovered_index);
        }
        // Update selection while dragging
        else if (state.is_dragging && ImGui::IsMouseDown(0)) {
          state.current_selection = calculate_selection(state.drag_start_index, hovered_index);
        }
      }
    }

    // End dragging when mouse is released
    if (state.is_dragging && ImGui::IsMouseReleased(0)) {
      state.is_dragging = false;

      // Update atlas drawIndex to the first selected tile
      if (!state.current_selection.indices.empty()) {
        atlas.drawIndex = state.current_selection.indices[0];
      }
    }

    // Draw grid lines and highlights
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (size_t i = 0; i < uv_bounds.size(); ++i) {
      float x0 = image_pos.x + uv_bounds[i][0] * display_width;
      float y0 = image_pos.y + uv_bounds[i][1] * preview_height;
      float x1 = image_pos.x + uv_bounds[i][2] * display_width;
      float y1 = image_pos.y + uv_bounds[i][3] * preview_height;

      // Check if this tile is in the current selection
      bool is_selected = std::find(state.current_selection.indices.begin(),
        state.current_selection.indices.end(),
        static_cast<int>(i)) != state.current_selection.indices.end();

      // Determine border color and thickness
      ImU32 border_color;
      float thickness;

      if (is_selected) {
        // Selected: green border
        border_color = IM_COL32(0, 255, 0, 255);
        thickness = 3.0f;
      }
      else {
        // Not selected: subtle white border
        border_color = IM_COL32(255, 255, 255, 180);
        thickness = 1.5f;
      }

      draw_list->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), border_color, 0.0f, 0, thickness);

      // Draw index number in the center of each cell
      char index_text[16];
      snprintf(index_text, sizeof(index_text), "%zu", i);
      ImVec2 text_size = ImGui::CalcTextSize(index_text);
      ImVec2 text_pos((x0 + x1) * 0.5f - text_size.x * 0.5f, (y0 + y1) * 0.5f - text_size.y * 0.5f);

      // Draw text background for better visibility
      ImVec2 text_bg_min(text_pos.x - 2, text_pos.y - 2);
      ImVec2 text_bg_max(text_pos.x + text_size.x + 2, text_pos.y + text_size.y + 2);
      draw_list->AddRectFilled(text_bg_min, text_bg_max, IM_COL32(0, 0, 0, 180));

      ImU32 text_color = is_selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255);
      draw_list->AddText(text_pos, text_color, index_text);
    }
  }

} // namespace ImGuiWidgets
