//-----------------------------------------------------------------------------------
// file:    TextureAtlasWidget.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Reusable ImGui widget for displaying and interacting with texture atlases.
//          Shows the full texture with grid overlays and allows selection of subtextures.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Graphics/Renderer.h"
#include <vector>
#include <array>
#include <functional>

namespace ImGuiWidgets {

  /*=================================================================================
  Function Brief:
  Displays a texture atlas with grid overlay showing all subtextures. The currently
  selected subtexture is highlighted in yellow. Allows dragging to change selection
  and calls a callback when the selection changes.

  Parameters:
  label             - Unique label for this widget (for ImGui ID)
  texture           - The atlas texture to display
  tex_coords_list   - Vector of UV coordinates for each subtexture (4 corners each)
  current_index     - [in/out] Currently selected subtexture index
  max_preview_size  - Maximum width/height for the preview (maintains aspect ratio)
  on_index_changed  - Optional callback called when selection changes

  Returns:
  true if the selection was changed this frame

  Example usage:
  std::vector<std::array<Math::Vec2D, 4>> coords = get_atlas_coords();
  int selected = 0;
  if (ImGuiWidgets::TextureAtlasPreview("MyAtlas", texture, coords, selected, 400.0f,
      [](int new_index) {
        std::cout << "Selected: " << new_index << std::endl;
      })) {
    // Selection changed this frame
  }
  ==================================================================================*/
  bool TextureAtlasPreview(
    const char* label,
    Engine::Graphics::Texture texture,
    const std::vector<std::array<Math::Vec2D, 4>>& tex_coords_list,
    int& current_index,
    float max_preview_size = 400.0f,
    std::function<void(int)> on_index_changed = nullptr
  );

} // namespace ImGuiWidgets
