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
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "TextureAtlasWidget.h"
#include <algorithm>

namespace ImGuiWidgets {

  bool TextureAtlasPreview(
    const char* label,
    Engine::Graphics::Texture texture,
    const std::vector<std::array<Math::Vec2D, 4>>& tex_coords_list,
    int& current_index,
    float max_preview_size,
    std::function<void(int)> on_index_changed
  ) {
    bool selection_changed = false;

    ImGui::PushID(label);

    // Validate inputs
    if (!texture || texture->id == 0) {
      ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No texture provided!");
      ImGui::PopID();
      return false;
    }

    if (tex_coords_list.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No subtextures defined!");
      ImGui::PopID();
      return false;
    }

    // Clamp current index
    if (current_index < 0) current_index = 0;
    if (current_index >= static_cast<int>(tex_coords_list.size())) {
      current_index = static_cast<int>(tex_coords_list.size()) - 1;
    }

    // Display info
    ImGui::Text("Total SubTextures: %zu", tex_coords_list.size());

    // Index selector with drag
    int new_index = current_index;
    if (ImGui::DragInt("Current Index", &new_index, 1.0f, 0, static_cast<int>(tex_coords_list.size()) - 1)) {
      if (new_index != current_index) {
        current_index = new_index;
        selection_changed = true;
        if (on_index_changed) {
          on_index_changed(current_index);
        }
      }
    }

    ImGui::Separator();

    // Calculate display size while maintaining aspect ratio
    float aspect_ratio = static_cast<float>(texture->width) / static_cast<float>(texture->height);
    float display_width = max_preview_size;
    float display_height = max_preview_size / aspect_ratio;

    if (display_height > max_preview_size) {
      display_height = max_preview_size;
      display_width = max_preview_size * aspect_ratio;
    }

    ImVec2 image_size(display_width, display_height);

    // Calculate UV bounds for each subtexture
    std::vector<std::array<float, 4>> uv_bounds; // x0, y0, x1, y1 for each subtexture

    for (size_t i = 0; i < tex_coords_list.size(); ++i) {
      const auto& coords = tex_coords_list[i];

      // Find min and max from the 4 corners
      float min_x = coords[0].x;
      float min_y = coords[0].y;
      float max_x = coords[0].x;
      float max_y = coords[0].y;

      for (size_t j = 1; j < 4; ++j) {
        min_x = std::min(min_x, coords[j].x);
        min_y = std::min(min_y, coords[j].y);
        max_x = std::max(max_x, coords[j].x);
        max_y = std::max(max_y, coords[j].y);
      }

      uv_bounds.push_back({ min_x, min_y, max_x, max_y });
    }

    // Get cursor position before drawing image
    ImVec2 image_pos = ImGui::GetCursorScreenPos();

    // Draw the texture
    ImGui::Image(
      (ImTextureID)(intptr_t)texture->id,
      image_size,
      ImVec2(0, 0),
      ImVec2(1, 1),
      ImVec4(1, 1, 1, 1),
      ImVec4(0, 0, 0, 0)
    );

    // Check for clicks on the image to select subtextures
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
      ImVec2 mouse_pos = ImGui::GetMousePos();
      float rel_x = (mouse_pos.x - image_pos.x) / display_width;
      float rel_y = (mouse_pos.y - image_pos.y) / display_height;

      // Find which subtexture was clicked
      for (size_t i = 0; i < uv_bounds.size(); ++i) {
        if (rel_x >= uv_bounds[i][0] && rel_x <= uv_bounds[i][2] &&
          rel_y >= uv_bounds[i][1] && rel_y <= uv_bounds[i][3]) {
          if (static_cast<int>(i) != current_index) {
            current_index = static_cast<int>(i);
            selection_changed = true;
            if (on_index_changed) {
              on_index_changed(current_index);
            }
          }
          break;
        }
      }
    }

    // Draw grid lines over the texture
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (size_t i = 0; i < uv_bounds.size(); ++i) {
      float x0 = image_pos.x + uv_bounds[i][0] * display_width;
      float y0 = image_pos.y + uv_bounds[i][1] * display_height;
      float x1 = image_pos.x + uv_bounds[i][2] * display_width;
      float y1 = image_pos.y + uv_bounds[i][3] * display_height;

      // Draw border for each cell
      ImU32 border_color = (i == static_cast<size_t>(current_index))
        ? IM_COL32(255, 255, 0, 255)  // Yellow for selected
        : IM_COL32(0, 255, 0, 180);    // Green for others

      float thickness = (i == static_cast<size_t>(current_index)) ? 3.0f : 1.5f;

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

      ImU32 text_color = (i == static_cast<size_t>(current_index))
        ? IM_COL32(255, 255, 0, 255)
        : IM_COL32(255, 255, 255, 255);
      draw_list->AddText(text_pos, text_color, index_text);
    }

    // Show detailed info about current subtexture
    if (current_index >= 0 && current_index < static_cast<int>(uv_bounds.size())) {
      ImGui::Separator();
      ImGui::Text("Selected SubTexture [%d]:", current_index);
      ImGui::Text("  UV Bounds: (%.3f, %.3f) -> (%.3f, %.3f)",
        uv_bounds[current_index][0], uv_bounds[current_index][1],
        uv_bounds[current_index][2], uv_bounds[current_index][3]);

      // Calculate pixel dimensions
      int pixel_width = static_cast<int>((uv_bounds[current_index][2] - uv_bounds[current_index][0]) * texture->width);
      int pixel_height = static_cast<int>((uv_bounds[current_index][3] - uv_bounds[current_index][1]) * texture->height);

      ImGui::Text("  Pixel Size: %d x %d", pixel_width, pixel_height);
    }

    ImGui::PopID();

    return selection_changed;
  }

} // namespace ImGuiWidgets
