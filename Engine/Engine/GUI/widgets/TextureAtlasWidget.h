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
//          Shows the full texture with grid overlays. Supports click-drag to select
//          single or multiple tiles in a rectangular region.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <vector>
#include "../../Components/Components.h"

namespace ImGuiWidgets {

  // Structure to hold information about tile selection (single or multiple)
  struct TileSelection {
    std::vector<int> indices;  // List of selected tile indices (sorted)
    int grid_width = 1;        // Width of selection in tiles
    int grid_height = 1;       // Height of selection in tiles
    int min_row = 0;           // Minimum row of selection
    int min_col = 0;           // Minimum column of selection

    bool is_single() const { return indices.size() == 1; }
    bool is_empty() const { return indices.empty(); }

    void clear() {
      indices.clear();
      grid_width = 1;
      grid_height = 1;
      min_row = 0;
      min_col = 0;
    }
  };

  // Per-widget state for maintaining selection across frames
  struct TextureAtlasState {
    bool is_dragging = false;
    int drag_start_index = -1;
    TileSelection current_selection;

    void reset() {
      is_dragging = false;
      drag_start_index = -1;
      current_selection.clear();
    }
  };

  void TextureAtlasPreview(
    const Components::Texture& texture,
    Components::TextureAtlas& atlas,
    float preview_height,
    TextureAtlasState& state
  );

} // namespace ImGuiWidgets
