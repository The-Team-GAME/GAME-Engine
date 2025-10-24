#include "SettingsFileIO.h"
#include <string>
#include <fstream>
#include <Logger.h>



namespace Engine {
  namespace FileIO {


    namespace {
      std::optional<Settings> load_settings_helper(const std::string& file_path) {
        std::ifstream file(file_path);

        if (!file.is_open())
        {
          Log::warning("Settings file not found: " + file_path);
          return Settings{};
        }

        std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        return ReflectionLibrary::from_json<Settings>(json_str);
      }
    }



    static Settings settings{};

    bool init_settings(const std::string& file_path)
    {
      auto settings_opt = load_settings_helper(file_path);

      if (!settings_opt.has_value())
      {
        // Save default settings if file not found or failed to load
        settings = Settings{};
        save_settings(file_path);
        return false;
      }

      Settings loaded_settings = settings_opt.value();
      Settings default_settings{};

      // Check for missing fields and set to default if necessary
      ReflectionLibrary::for_each_field(loaded_settings, [&](auto field_ptr, const char* field_name) {

        });

      return true;
    }


    bool load_settings(const std::string& file_path)
    {
      auto settings_opt = load_settings_helper(file_path);

      if (!settings_opt.has_value()) return false;

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


