#pragma once

#include "EngineExport.h"
#include <string>
#include <filesystem>

namespace Engine
{
  namespace GameProjectScaffolder
  {
    /**
     * @brief Creates a new game project from the _Template.
     * @param game_name Name for the new game (used for folder and .sln name).
     * @return True if the project was created successfully.
     *
     * Creates: Games/<game_name>/
     *            GameLogic/       (copied from _Template, GUID replaced)
     *            Assets/settings/ (empty settings ready for use)
     *            <game_name>.sln  (references MonkeyEngine + new GameLogic)
     */
    ENGINE_API bool create_project(const std::string& game_name);
  }
}
