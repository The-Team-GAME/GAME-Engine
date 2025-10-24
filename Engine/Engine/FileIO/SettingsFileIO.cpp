#include "SettingsFileIO.h"
#include <string>
#include <fstream>

namespace Engine {
  namespace FileIO {

    static Settings settings{};

    bool load_settings(const std::string& file_path)
    {
      std::ifstream file(file_path);

      if (!file.is_open())
      {
        return false;
      }

      std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

      auto settings_opt = ReflectionLibrary::from_json<Settings>(json_str);

      if (!settings_opt.has_value())
      {
        return false;
      }

      settings = settings_opt.value();

      return true;
    }

    bool save_settings(const std::string& file_path)
    {
      std::ofstream file(file_path);
      if (!file.is_open())
      {
        return false;
      }
      std::string json_str = ReflectionLibrary::to_json<Settings>(settings, true);
      file << json_str;
      return true;
    }
  }
}


