#pragma once

#include <string>
#include <vector>
#include "Reflection/ReflectionLibrary/GenericValue.h"
#include "ECS/ECS.h"


namespace Engine {

  struct Level {
    std::vector<ReflectionLibrary::GenericValue> entities;

    static Level from_world();

    void instantiate_into_world();

  };

  namespace LevelManager {
    std::string get_current_level_path();
    void set_current_level_path(const std::string& level_name); // Only used in SceneManager, when loading scenes instead of levels
  }

}