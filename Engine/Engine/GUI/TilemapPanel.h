//-----------------------------------------------------------------------------------
// file:    TilemapPanel.h
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   This file defines the TilemapPanel ImGui interface for painting tiles
//          directly on the world using TextureAtlas. Allows selection of tiles
//          from an atlas and placement in the scene as entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"
#include <string>

namespace TilemapPanel {

  /*=================================================================================
  Function Brief:
  Initializes the tilemap panel by loading saved tilemaps from project settings.
  Must be called once during engine initialization before using the panel.

  Where to call it:
  Called during engine initialization after project settings have been loaded.
  ==================================================================================*/
  void init();

  /*=================================================================================
  Function Brief:
  Renders the ImGui panel UI for the tilemap editor. Displays texture atlas
  selection, tile preview, painting controls, grid settings, and statistics.

  Where to call it:
  Called during the ImGui phase of the main game loop, before rendering begins.
  ==================================================================================*/
  void draw_panel();

  /*=================================================================================
  Function Brief:
  Handles mouse input for tile placement and deletion. Processes clicks when not
  over ImGui windows and places or erases tiles based on the current mode.

  Where to call it:
  Called during the update/input phase of the game loop, before rendering.
  ==================================================================================*/
  void update();

  /*=================================================================================
  Function Brief:
  Draws the grid overlay and current cell highlight in world space. Renders grid
  lines and highlights the cell under the mouse cursor when painting/erasing.

  Where to call it:
  Called during the rendering phase after begin_batch() has been called and before
  end_batch().
  ==================================================================================*/
  void draw_overlay();

  /* =============================  Helper Functions  ============================= */

  /*=================================================================================
  Function Brief:
  Regenerates the texture atlas based on the current selected texture and configured
  rows/columns. Updates the atlas subdivision and resets the selected tile index.

  Where to call it:
  Called internally when a new texture is selected or when atlas rows/columns are
  changed.
  ==================================================================================*/
  void regenerate_atlas();

  /*=================================================================================
  Function Brief:
  Generates a unique string key for a grid position. Used to track placed tiles
  and prevent duplicate placement at the same location.

  Parameters:
  pos - World position to convert to grid coordinates

  Returns:
  String in format "gridX_gridY"

  Where to call it:
  Called internally when placing, checking for, or deleting tiles.
  ==================================================================================*/
  std::string get_position_key(const Math::Vec2D& pos);

  /*=================================================================================
  Function Brief:
  Snaps a world position to the center of the nearest grid cell, accounting for
  grid offset. Returns the center point of the grid cell.

  Parameters:
  pos - World position to snap

  Returns:
  World position at the center of the grid cell

  Where to call it:
  Called when placing tiles, highlighting grid cells, or updating tile positions.
  ==================================================================================*/
  Math::Vec2D snap_to_grid(const Math::Vec2D& pos);

  /*=================================================================================
  Function Brief:
  Deletes a tile entity at the specified grid position. Only deletes entities
  that have the Tile component to ensure only tilemap tiles are removed.

  Parameters:
  grid_pos - Snapped grid position to delete tile from

  Where to call it:
  Called when erasing mode is enabled and the user clicks in the world.
  ==================================================================================*/
  void delete_tile_at_position(const Math::Vec2D& grid_pos);

  /*=================================================================================
  Function Brief:
  Updates all existing tile entities when grid settings change. Re-snaps all tile
  positions to the new grid and updates their scale to match the new grid_snap size.
  Rebuilds the placed_tiles tracking set.

  Where to call it:
  Called internally when grid_snap or grid_offset values are modified.
  ==================================================================================*/
  void update_all_tile_dimensions();

  /*=================================================================================
  Function Brief:
  Creates a new tile entity at the specified world position. Snaps the position to
  the grid, checks for duplicates, and creates an entity with Tile, Transform2D,
  Texture, TextureAtlas, and Color components. If multi-tile selection is active,
  places multiple tiles in a pattern.

  Parameters:
  world_pos - Mouse world position where the user clicked

  Where to call it:
  Called when painting mode is enabled and the user clicks in the world.
  ==================================================================================*/
  void place_tile_at_position(const Math::Vec2D& world_pos);

  /*=================================================================================
  Function Brief:
  Returns whether tilemap editing mode is currently enabled.

  Returns:
  True if tilemap mode is enabled, false otherwise.
  ==================================================================================*/
  bool is_tilemap_mode_enabled();

  /*=================================================================================
  Function Brief:
  Returns the name of the currently selected layer in the tilemap editor.

  Returns:
  String name of the selected layer.
  ==================================================================================*/
  std::string get_selected_layer();

  /*=================================================================================
  Function Brief:
  Returns whether the "dim non-selected layers" feature is currently enabled.
  When enabled, layers other than the selected one are rendered at reduced brightness.

  Returns:
  True if dimming is enabled, false otherwise.
  ==================================================================================*/
  bool is_dim_non_selected_layers_enabled();
}
