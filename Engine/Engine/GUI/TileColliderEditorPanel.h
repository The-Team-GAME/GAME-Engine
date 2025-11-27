//-----------------------------------------------------------------------------------
// file:    TileColliderEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the TileColliderEditorPanel ImGui interface for editing
//          tile collider configurations. Shows texture atlas on the left for tile
//          selection and collider editing on the right (AABB or Custom polygon).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../Graphics/RendererInternal.h"

namespace TileColliderEditor {

  /*=================================================================================
  Function Brief:
  Opens the tile collider editor for a specific atlas from the loaded tilemaps.

  Parameters:
  atlas_index - Index of the atlas in the project settings loaded_tilemaps array

  Where to call it:
  Called from the tilemap panel or other UI when user wants to edit tile colliders.
  ==================================================================================*/
  void Open(int atlas_index);

}

/*=================================================================================
Function Brief:
Renders the Tile Collider Editor panel. Shows atlas preview on the left for
tile selection, and collider editing interface on the right. Allows switching
between Default (AABB merged) and Custom (polygon) collider types per tile.

Where to call it:
Register this with ImGuiManager to be called every frame.
==================================================================================*/
void TileColliderEditorPanel();
