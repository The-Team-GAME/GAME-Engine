#include "GameProjectScaffolder.h"
#include "GameProjectManager.h"
#include "Logger.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <objbase.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>

namespace fs = std::filesystem;

namespace Engine
{
  namespace GameProjectScaffolder
  {
    // Generate a GUID string like "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" (uppercase)
    static std::string generate_guid()
    {
      GUID guid;
      if (FAILED(CoCreateGuid(&guid)))
      {
        Log::error("[Scaffolder] CoCreateGuid failed");
        return "00000000-0000-0000-0000-000000000000";
      }

      char buf[40];
      snprintf(buf, sizeof(buf),
        "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
      return std::string(buf);
    }

    static std::string read_file_text(const fs::path& path)
    {
      std::ifstream in(path, std::ios::binary);
      if (!in) return "";
      std::ostringstream ss;
      ss << in.rdbuf();
      return ss.str();
    }

    static bool write_file_text(const fs::path& path, const std::string& content)
    {
      std::ofstream out(path, std::ios::binary);
      if (!out) return false;
      out << content;
      return true;
    }

    // Replace all occurrences of `from` with `to` in `str`
    static void replace_all(std::string& str, const std::string& from, const std::string& to)
    {
      size_t pos = 0;
      while ((pos = str.find(from, pos)) != std::string::npos)
      {
        str.replace(pos, from.size(), to);
        pos += to.size();
      }
    }

    bool create_project(const std::string& game_name)
    {
      if (game_name.empty() || game_name[0] == '_')
      {
        Log::error("[Scaffolder] Invalid game name: '{}'", game_name);
        return false;
      }

      auto workspace_root = GameProjectManager::get_workspace_root();
      auto template_dir = workspace_root / "Games" / "_Template";
      auto target_dir = workspace_root / "Games" / game_name;

      if (!fs::exists(template_dir))
      {
        Log::error("[Scaffolder] Template not found at: {}", template_dir.string());
        return false;
      }

      if (fs::exists(target_dir))
      {
        Log::error("[Scaffolder] Game '{}' already exists at: {}", game_name, target_dir.string());
        return false;
      }

      Log::info("[Scaffolder] Creating game project: {}", game_name);

      // Copy template directory tree
      std::error_code ec;
      fs::copy(template_dir, target_dir, fs::copy_options::recursive, ec);
      if (ec)
      {
        Log::error("[Scaffolder] Failed to copy template: {}", ec.message());
        return false;
      }

      // Generate unique GUIDs
      auto proj_guid = generate_guid();
      auto sln_guid = generate_guid();

      // Process .vcxproj — replace GUID placeholder
      auto vcxproj_path = target_dir / "GameLogic" / "GameLogic.vcxproj";
      if (fs::exists(vcxproj_path))
      {
        auto content = read_file_text(vcxproj_path);
        replace_all(content, "__TEMPLATE_GUID__", proj_guid);
        write_file_text(vcxproj_path, content);
      }

      // Rename and process .sln
      auto template_sln = target_dir / "_Template.sln";
      auto target_sln = target_dir / (game_name + ".sln");

      if (fs::exists(template_sln))
      {
        auto content = read_file_text(template_sln);
        replace_all(content, "__TEMPLATE_GUID__", proj_guid);
        replace_all(content, "__TEMPLATE_SLN_GUID__", sln_guid);
        write_file_text(target_sln, content);
        fs::remove(template_sln, ec);
      }

      Log::info("[Scaffolder] Game project '{}' created successfully at: {}", game_name, target_dir.string());
      return true;
    }
  }
}
