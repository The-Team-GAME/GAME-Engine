// This file contains functions to manage project save data

#pragma once

#include <variant>
#include <string>


namespace Engine {
  namespace ProjectSaveManager {

    // Different types of dirty states, each with their own data
    struct ProjectSettingsDirty {};

    struct LevelDirty {
      std::string level_name;
      explicit LevelDirty(std::string name) : level_name(std::move(name)) {}
    };

    // Variant that can hold any dirty state type
    using DirtyState = std::variant<ProjectSettingsDirty, LevelDirty>;

    void set_dirty(DirtyState state);

    void save_all_dirty();

    bool get_is_any_dirty();

  }
}