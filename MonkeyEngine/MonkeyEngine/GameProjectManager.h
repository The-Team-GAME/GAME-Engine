#pragma once

#include "EngineExport.h"
#include <string>
#include <vector>
#include <filesystem>

namespace Engine
{
  struct GameProject
  {
    std::string name;
    std::filesystem::path root_dir;       // e.g. Games/MonkeyStoleMyGPA/
    std::filesystem::path sln_path;       // e.g. Games/MonkeyStoleMyGPA/MonkeyStoleMyGPA.sln
    std::filesystem::path vcxproj_path;   // e.g. Games/MonkeyStoleMyGPA/GameLogic/GameLogic.vcxproj
    std::filesystem::path source_dir;     // e.g. Games/MonkeyStoleMyGPA/GameLogic/
  };

  namespace GameProjectManager
  {
    ENGINE_API std::filesystem::path get_workspace_root();

    ENGINE_API std::vector<GameProject> discover_games();

    ENGINE_API const std::string& get_current_game_name();

    ENGINE_API bool switch_game(const std::string& game_name);

    ENGINE_API void close_game();
  }
}
