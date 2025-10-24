#include "SettingsFileIO.h"
#include <string>
#include <fstream>
#include <Logger.h>



namespace Engine {
  namespace FileIO {


    namespace {
      std::optional<ReflectionLibrary::GenericValue> load_settings_helper(const std::string& file_path) {
        std::ifstream file(file_path);

        if (!file.is_open())
        {
          Log::warning("Settings file not found: " + file_path);
          return ReflectionLibrary::GenericValue{};
        }

        std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        return ReflectionLibrary::generic_value_from_json(json_str);
      }
    }



    static Settings settings{};

    bool load_settings(const std::string& file_path)
    {
      // Load existing settings
      auto loaded_settings_opt_value = load_settings_helper(file_path);
      if (!loaded_settings_opt_value.has_value()) {
        Log::error("Failed to load settings");
        return false;
      }

      ReflectionLibrary::GenericValue loaded_settings = loaded_settings_opt_value.value();
      ReflectionLibrary::GenericValue default_settings = ReflectionLibrary::to_generic_value(Settings{});

      bool has_missing = false; // Only save if there are missing fields
      for (const auto& [key, default_value] : default_settings.getObject()) // Iterate through default settings
      {
        // If key is missing in loaded settings, add it with default value
        if (!loaded_settings.has(key))
        {
          loaded_settings.getObject()[key] = default_value;
          has_missing = true;
        }
      }

      if (has_missing)
      {
        // Save updated settings back to file
        if (!save_settings(file_path))
        {
          Log::warning("Failed to save updated settings to file: " + file_path);
          // Proceeding even if save fails
        }
      }

      auto loaded_settings_opt = ReflectionLibrary::from_generic_value<Settings>(loaded_settings);

      if (!loaded_settings_opt.has_value())
      {
        Log::error("Failed to parse loaded settings");
        return false;
      }

      // Update global settings
      settings = loaded_settings_opt.value();

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


