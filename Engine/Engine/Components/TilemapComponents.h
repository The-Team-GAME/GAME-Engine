#include "../ECS/Macros.h"

namespace Components {
  struct Tile {};

  // Only used in TilemapPanel and ProjectSettings for saving/loading tilemaps
  struct AtlasEntry {
    Components::Texture texture;
    Components::TextureAtlas atlas;
    std::string name;
  };
}



REGISTER_COMPONENT(Components::Tile);
REGISTER_COMPONENT(Components::AtlasEntry,
  texture,
  atlas,
  name
);