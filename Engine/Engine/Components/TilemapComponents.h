//-----------------------------------------------------------------------------------
// file:    TilemapComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for the tilemap system. Includes Tile
//          component for marking tile entities and AtlasEntry for storing
//          texture atlas information used in tilemap editing and serialization.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/Macros.h"

namespace Components {
  struct Tile {};

  // Only used in TilemapPanel and ProjectSettings for saving/loading tilemaps
  struct AtlasEntry {
    Components::Texture texture;
    Components::TextureAtlas atlas;
  };
}

REGISTER_COMPONENT(Components::Tile);
REGISTER_COMPONENT(Components::AtlasEntry, texture, atlas);