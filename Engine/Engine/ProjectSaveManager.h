// This file contains functions to manage project save data

#pragma once

#include <variant>
#include <string>
#include "ECS/Prefab.h"


namespace Engine {
  namespace ProjectSaveManager {


    // Different types of dirty states, each with their own data
    namespace DirtyStates {
      struct ProjectSettings {};
      struct Level {
        std::string level_path;
        Level(const std::string& level_path) : level_path(level_path) {}
      };
      struct Prefab {
        std::string prefab_path;
        Prefab(const std::string& path) : prefab_path(path) {}
      };
    }

    // Variant that can hold any dirty state type
    using DirtyState = std::variant<DirtyStates::ProjectSettings, DirtyStates::Level, DirtyStates::Prefab>;

    void set_dirty(DirtyState state);

    bool check_dirty(const DirtyState& state);

    void save_all_dirty();

    bool is_any_dirty();

  }
}