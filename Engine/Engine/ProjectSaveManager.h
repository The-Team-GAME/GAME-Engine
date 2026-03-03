//-----------------------------------------------------------------------------------
// file:    ProjectSaveManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ProjectSaveManager namespace and DirtyStates, providing a system to track unsaved changes in the project and save them to disk.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
// This file contains functions to manage project save data

#pragma once

#include <variant>
#include <string>
#include <vector>


namespace Engine {
  namespace ProjectSaveManager {


    // Different types of dirty states, each with their own data
    namespace DirtyStates {
      struct ProjectSettings {};
      struct Level {
        std::string level_path;
        Level(const std::string& level_path) : level_path(level_path) {}
      };
    }

    // Variant that can hold any dirty state type
    using DirtyState = std::variant<DirtyStates::ProjectSettings, DirtyStates::Level>;

    void set_dirty(DirtyState state);

    bool check_dirty(const DirtyState& state);

    void save_all_dirty();

    bool is_any_dirty();

    // Get the list of all currently dirty states
    const std::vector<DirtyState>& get_dirty_states();

    // Get a human-readable display name for a dirty state
    std::string get_dirty_display_name(const DirtyState& state);

    // Save a single dirty state and remove it from the dirty list
    void save_single_dirty(size_t index);

  }
}