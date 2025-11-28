//-----------------------------------------------------------------------------------
// file:    LevelComponent.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Component that marks an entity as a Level container.
//          Level entities serve as the root parent for all entities within a loaded level.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../ECS/ECS.h"
#include "../ECS/Macros.h"

namespace Components
{
  struct Level {
    std::string level_path;
    
    Level() = default;
    Level(const std::string& path) : level_path(path) {}
  };
}

REGISTER_COMPONENT(Components::Level, level_path)
