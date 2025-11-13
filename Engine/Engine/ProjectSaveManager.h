// This file contains functions to manage project save data

#pragma once

#include <bitset>


namespace Engine {
  namespace ProjectSaveManager {
    enum class Dirty {
      ProjectSettings = 0,
    };

    void set_dirty(Dirty type);

    void save_all_dirty();

    bool get_is_any_dirty();

  }
}