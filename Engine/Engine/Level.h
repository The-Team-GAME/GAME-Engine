#pragma once

#include <string>
#include <vector>
#include "Reflection/ReflectionLibrary/GenericValue.h"
#include "ECS/ECS.h"


namespace Engine {

  struct Level {
    std::string name;
    std::vector<ReflectionLibrary::GenericValue> entities;

    static Level from_world(const std::string& level_name = "");

    void instantiate_into_world();

  };

  namespace LevelManager {
    std::string get_current_level_name();
    void set_current_level_name(const std::string& level_name); // Only used in SceneManager, when loading scenes instead of levels
  }

}