#include "SettingsManager.h"
#include <string>
#include <fstream>
#include "../Logger.h"
#include <filesystem>

namespace Engine {
  namespace FileIO {
    template<typename T>
    bool SettingsManager<T>::load(const std::string& file_path)
    {
      std::ifstream file(file_path);

      if (!file.is_open())
      {
        Log::error("Settings file not found: " + file_path);
        return false;
      }

      std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

      auto loaded_settings_opt_value = ReflectionLibrary::generic_value_from_json(json_str);
      if (!loaded_settings_opt_value.has_value()) {
        Log::error("Failed to load settings");
        return false;
      }

      ReflectionLibrary::GenericValue loaded_settings = loaded_settings_opt_value.value();
      ReflectionLibrary::GenericValue default_settings = ReflectionLibrary::to_generic_value(T{});

      bool has_missing = false;
      for (const auto& [key, default_value] : default_settings.getObject())
      {
        // Check if the key is missing in loaded settings
        if (!loaded_settings.has(key))
        {
          loaded_settings.getObject()[key] = default_value;
          has_missing = true;
        }
      }

      if (has_missing)
      {
        if (!save(file_path))
        {
          Log::warning("Failed to save updated settings to file: " + file_path);
        }
      }

      auto loaded_settings_opt = ReflectionLibrary::from_generic_value<T>(loaded_settings);

      if (!loaded_settings_opt.has_value())
      {
        Log::error("Failed to parse loaded settings");
        return false;
      }

      settings = loaded_settings_opt.value();
      return true;
    }

    template<typename T>
    bool SettingsManager<T>::save(const std::string& file_path)
    {
      // Extract directory path
      size_t last_slash = file_path.find_last_of("/\\");
      if (last_slash != std::string::npos)
      {
        std::string directory = file_path.substr(0, last_slash);

        // Check if directory exists, create if it doesn't
        if (!std::filesystem::exists(directory))
        {
          try
          {
            std::filesystem::create_directories(directory);
          }
          catch (const std::filesystem::filesystem_error& e)
          {
            Log::error("Failed to create directory: " + directory + " - " + e.what());
            return false;
          }
        }
      }

      // Write settings to file
      std::ofstream file(file_path);
      if (!file.is_open())
      {
        return false;
      }
      std::string json_str = ReflectionLibrary::to_json<T>(settings, true);
      file << json_str;
      return true;
    }

  }
}