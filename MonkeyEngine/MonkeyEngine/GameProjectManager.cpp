#include "GameProjectManager.h"
#include "HotReloadManager.h"
#include "SceneManager.h"
#include "Resources.h"
#include "Logger.h"

#include <filesystem>

namespace Engine
{
  namespace GameProjectManager
  {
    static std::string s_current_game;

    std::filesystem::path get_workspace_root()
    {
      auto dir = HotReload::get_solution_dir();
      // Walk up from solution dir until we find Directory.Build.props
      while (!dir.empty() && dir.has_parent_path())
      {
        if (std::filesystem::exists(dir / "Directory.Build.props"))
          return dir;
        auto parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
      }
      // Fallback: assume workspace root is two levels up from solution dir
      return HotReload::get_solution_dir().parent_path().parent_path();
    }

    std::vector<GameProject> discover_games()
    {
      std::vector<GameProject> games;
      auto games_dir = get_workspace_root() / "Games";

      if (!std::filesystem::exists(games_dir))
        return games;

      for (auto& entry : std::filesystem::directory_iterator(games_dir))
      {
        if (!entry.is_directory()) continue;

        auto name = entry.path().filename().string();
        if (name.empty() || name[0] == '_') continue; // skip _Template

        auto vcxproj = entry.path() / "GameLogic" / "GameLogic.vcxproj";
        if (!std::filesystem::exists(vcxproj)) continue;

        GameProject proj;
        proj.name = name;
        proj.root_dir = entry.path();
        proj.source_dir = entry.path() / "GameLogic";
        proj.vcxproj_path = vcxproj;

        // Find .sln file
        for (auto& sln_entry : std::filesystem::directory_iterator(entry.path()))
        {
          if (sln_entry.path().extension() == ".sln")
          {
            proj.sln_path = sln_entry.path();
            break;
          }
        }

        games.push_back(std::move(proj));
      }
      return games;
    }

    const std::string& get_current_game_name()
    {
      return s_current_game;
    }

    bool switch_game(const std::string& game_name)
    {
      auto games = discover_games();
      const GameProject* target = nullptr;
      for (auto& g : games)
      {
        if (g.name == game_name)
        {
          target = &g;
          break;
        }
      }
      if (!target)
      {
        Log::error("[GameProjectManager] Game '{}' not found", game_name);
        return false;
      }

      Log::info("[GameProjectManager] Switching to game: {}", game_name);

#ifndef GAME_BUILD
      // Stop source watcher for old game
      HotReload::stop_build();
      HotReload::stop_source_watcher();
#endif

      // Unload current DLL
      HotReload::unload_game_logic();

      // Point HotReload at the new game's source
      HotReload::set_game_vcxproj(target->vcxproj_path);
      HotReload::set_game_source_dir(target->source_dir);

      s_current_game = game_name;

#ifndef GAME_BUILD
      // Always rebuild when switching games — a stale DLL may have been
      // linked against a different engine import library.
      Log::info("[GameProjectManager] Building GameLogic for {}...", game_name);
      HotReload::build_game_logic();
#endif

      // Load the new game DLL
      if (!HotReload::load_game_logic())
      {
        Log::error("[GameProjectManager] Failed to load GameLogic for {}", game_name);
        return false;
      }

      // Reload prefabs for the new game
      Engine::Resources::load_all_prefabs();

#ifndef GAME_BUILD
      // Start watching new game's source files
      HotReload::start_source_watcher();
#endif

      Log::info("[GameProjectManager] Switched to game: {}", game_name);
      return true;
    }

    void close_game()
    {
      if (s_current_game.empty())
        return;

      Log::info("[GameProjectManager] Closing game: {}", s_current_game);

#ifndef GAME_BUILD
      HotReload::stop_build();
      HotReload::stop_source_watcher();
#endif

      HotReload::unload_game_logic();
      s_current_game.clear();

      Log::info("[GameProjectManager] Game closed — editor is now empty");
    }
  }
}
