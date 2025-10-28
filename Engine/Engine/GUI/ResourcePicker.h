//-----------------------------------------------------------------------------------
// file:    ResourcePicker.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines ImGui resource picker widgets for selecting textures,
//          fonts, and other resources with visual previews in a grid layout.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../Graphics/Renderer.h"

namespace ResourcePicker {

  /*=================================================================================
  Function Brief:
  Displays a texture picker popup with grid preview of all loaded textures.
  Returns true and sets selected_path if user makes a selection.

  Parameters:
  popup_id       - Unique ID for the popup window
  selected_path  - [out] Path of selected texture (if function returns true)
  current_path   - Currently selected texture path (for highlighting)
  grid_columns   - Number of columns in the preview grid
  preview_size   - Size of each texture preview thumbnail

  Where to call it:
  After calling ImGui::OpenPopup() with matching popup_id.
  ==================================================================================*/
  bool texture_picker_popup(const char* popup_id, std::string& selected_path, 
                           const std::string& current_path = "",
                           int grid_columns = 4, float preview_size = 64.0f);

  /*=================================================================================
  Function Brief:
  Displays a font picker popup with preview of all loaded fonts.
  Returns true and sets selected_path and selected_size if user makes a selection.

  Parameters:
  popup_id       - Unique ID for the popup window
  selected_path  - [out] Path of selected font (if function returns true)
  selected_size  - [out] Size of selected font (if function returns true)
  current_path   - Currently selected font path (for highlighting)
  current_size   - Currently selected font size (for highlighting)

  Where to call it:
  After calling ImGui::OpenPopup() with matching popup_id.
  ==================================================================================*/
  bool font_picker_popup(const char* popup_id, std::string& selected_path, 
                        int& selected_size, const std::string& current_path = "",
                        int current_size = 0);

  /*=================================================================================
  Function Brief:
  Gets all currently loaded textures from the resource cache.

  Parameters:
  None

  Returns:
  Vector of Texture shared pointers containing all loaded textures.
  ==================================================================================*/
  std::vector<Engine::Graphics::Texture> get_all_loaded_textures();

  /*=================================================================================
  Function Brief:
  Gets all currently loaded fonts from the resource cache.

  Parameters:
  None

  Returns:
  Vector of Font shared pointers containing all loaded fonts.
  ==================================================================================*/
  std::vector<Engine::Graphics::Font> get_all_loaded_fonts();

} // namespace ResourcePicker
