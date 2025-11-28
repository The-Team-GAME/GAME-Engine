//-----------------------------------------------------------------------------------
// file:    TilemapManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Core tilemap management system that handles TileMap-based tilemap
//          storage, tile placement/deletion, and automatic merging of rectangular
//          tile regions for optimized rendering.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Math/Vector2D.h"
#include "Components/Components.h"
#include "ECS/ECS.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace TilemapManager {

  /*=================================================================================
  Function Brief:
  Initializes the tilemap manager by loading existing TileMap entities and
  rebuilding their visual tile representations.

  Where to call it:
  Called during engine initialization or when loading a level.
  ==================================================================================*/
  void init();

  /*=================================================================================
  Function Brief:
  Sets the grid size for tile snapping and updates all existing tilemaps.

  Parameters:
  size - The size of each grid cell in world units

  Where to call it:
  When the user changes the grid size in the tilemap editor.
  ==================================================================================*/
  void set_grid_size(float size);

  /*=================================================================================
  Function Brief:
  Gets the current grid size.

  Returns:
  The current grid size in world units.
  ==================================================================================*/
  float get_grid_size();

  /*=================================================================================
  Function Brief:
  Sets the grid offset for tile positioning.

  Parameters:
  offset - The offset from origin in world coordinates

  Where to call it:
  When the user changes the grid offset in the tilemap editor.
  ==================================================================================*/
  void set_grid_offset(const Math::Vec2D& offset);

  /*=================================================================================
  Function Brief:
  Gets the current grid offset.

  Returns:
  The current grid offset in world coordinates.
  ==================================================================================*/
  Math::Vec2D get_grid_offset();

  /*=================================================================================
  Function Brief:
  Places a tile at the specified world position on the given layer.

  Parameters:
  world_pos - World position where the tile should be placed
  layer_name - Name of the layer to place the tile on
  atlas_index - Index of the atlas in the loaded tilemaps
  tile_index - Index of the tile within the atlas

  Where to call it:
  When the user paints a tile in the editor.
  ==================================================================================*/
  void place_tile(const Math::Vec2D& world_pos, const std::string& layer_name, int atlas_index, int tile_index);

  /*=================================================================================
  Function Brief:
  Deletes the tile at the specified world position on the given layer.

  Parameters:
  world_pos - World position of the tile to delete
  layer_name - Name of the layer to delete the tile from

  Where to call it:
  When the user erases a tile in the editor.
  ==================================================================================*/
  void delete_tile(const Math::Vec2D& world_pos, const std::string& layer_name);

  /*=================================================================================
  Function Brief:
  Snaps a world position to the nearest grid cell center.

  Parameters:
  pos - World position to snap

  Returns:
  The snapped position at the center of the nearest grid cell.
  ==================================================================================*/
  Math::Vec2D snap_to_grid(const Math::Vec2D& pos);

  /*=================================================================================
  Function Brief:
  Converts a world position to grid coordinates.

  Parameters:
  world_pos - World position to convert
  grid_x - Output parameter for grid X coordinate
  grid_y - Output parameter for grid Y coordinate
  ==================================================================================*/
  void world_to_grid(const Math::Vec2D& world_pos, int& grid_x, int& grid_y);

  /*=================================================================================
  Function Brief:
  Converts grid coordinates to world position (center of tile).

  Parameters:
  grid_x - Grid X coordinate
  grid_y - Grid Y coordinate

  Returns:
  World position at the center of the grid cell.
  ==================================================================================*/
  Math::Vec2D grid_to_world(int grid_x, int grid_y);

  /*=================================================================================
  Function Brief:
  Rebuilds all tile entities from TileMap data for the specified layer.
  This merges rectangular regions of the same tile into single entities for
  optimized rendering.

  Parameters:
  layer_name - Name of the layer to rebuild

  Where to call it:
  Called automatically after placing or deleting tiles, or when loading a level.
  ==================================================================================*/
  void rebuild_tilemap_for_layer(const std::string& layer_name);

  /*=================================================================================
  Function Brief:
  Gets or creates the TileMap entity for the specified layer.

  Parameters:
  layer_name - Name of the layer

  Returns:
  The TileMap entity for this layer.
  ==================================================================================*/
  Engine::ECS::Entity get_or_create_tilemap_entity(const std::string& layer_name);

  /*=================================================================================
  Function Brief:
  Gets the tile at the specified grid position on the given layer.

  Parameters:
  layer_name - Name of the layer to query
  grid_x - Grid X coordinate
  grid_y - Grid Y coordinate

  Returns:
  The Tile at that position, or an invalid Tile if no tile exists.
  ==================================================================================*/
  Engine::Graphics::Tile get_tile_at(const std::string& layer_name, int grid_x, int grid_y);

  /*=================================================================================
  Function Brief:
  Updates all tilemap dimensions when grid size or offset changes.
  ==================================================================================*/
  void update_all_tile_dimensions();

  /*=================================================================================
  Function Brief:
  Calculates the world bounds of all active tilemaps.

  Parameters:
  min - Output parameter for the minimum world coordinates (bottom-left)
  max - Output parameter for the maximum world coordinates (top-right)

  Returns:
  True if any tilemaps were found and bounds calculated, false otherwise.
  ==================================================================================*/
  bool get_world_bounds(Math::Vec2D& min, Math::Vec2D& max);

  /*=================================================================================
  Function Brief:
  Calculates the world bounds and updates the collision system.
  If no tilemaps exist, it uses the default screen bounds.
  ==================================================================================*/
  void broadcast_world_bounds();

  /*=================================================================================
  Function Brief:
  Moves all tiles from the source layer to the destination layer.

  Parameters:
  source_layer - Name of the layer to move tiles from
  dest_layer - Name of the layer to move tiles to

  Where to call it:
  When the user wants to reorganize tiles between layers.
  ==================================================================================*/
  void move_all_tiles_to_layer(const std::string& source_layer, const std::string& dest_layer);

} // namespace TilemapManager
