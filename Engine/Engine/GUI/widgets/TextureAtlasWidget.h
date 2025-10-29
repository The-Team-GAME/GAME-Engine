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
 
#include <functional>
#include "../../Components/Components.h"

namespace ImGuiWidgets {

  bool TextureAtlasPreview(
    const char* label,
    Components::Texture& texture,
    Components::TextureAtlas& atlas,
    float max_preview_size,
    std::function<void(int)> on_index_changed = nullptr
  );

} // namespace ImGuiWidgets
