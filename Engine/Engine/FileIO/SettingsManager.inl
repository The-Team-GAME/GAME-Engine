//-----------------------------------------------------------------------------------
// file:    SettingsManager.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the SettingsManager template class which handles
//          loading and saving of settings to/from JSON files. Supports automatic
//          creation of default settings and merging missing fields from defaults.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "SettingsManager.h"
#include <string>
#include <fstream>
#include "../Logger.h"
#include <filesystem>
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace FileIO {

    template<typename T>
    bool SettingsManager<T>::load_impl(const std::string& file_path)
    {
      std::ifstream file(file_path);

      if (!file.is_open())
      {
        Log::info("Settings file not found, creating: {}", file_path);

        settings_ = T{};
        if (!save_impl(file_path))
        {
          Log::warning("Failed to create default settings file: {}", file_path);
        }

        return true;
      }

      std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

      auto loaded_settings_opt_value = ReflectionLibrary::generic_value_from_json(json_str);
      if (!loaded_settings_opt_value.has_value()) {
        Log::error("Failed to load settings");
        return false;
      }

      ReflectionLibrary::GenericValue loaded_settings = loaded_settings_opt_value.value();
      ReflectionLibrary::GenericValue default_settings = ReflectionLibrary::to_generic_value(T{});

      // Ensure both loaded_settings and default_settings are objects
      if (!default_settings.isObject())
      {
        Log::error("Failed to convert default settings to object, are you sure {} is reflectable?", typeid(T).name());
        return false;
      }

      if (!loaded_settings.isObject())
      {
        Log::warning("Loaded settings is not an object, using defaults");
        settings_ = T{};
        if (!save_impl(file_path))
        {
          Log::warning("Failed to save default settings to file: {}", file_path);
        }
        return true;
      }

      bool has_missing = false;
      for (const auto& [key, default_value] : default_settings.getObject())
      {
        Log::info("Checking setting key: {}", key);
        // Check if the key is missing in loaded settings
        if (!loaded_settings.has(key))
        {
          Log::info("Missing setting '{}' in file. Adding default value.", key);
          loaded_settings.getObject()[key] = default_value;
          has_missing = true;
        }
      }

      if (has_missing)
      {
        if (!save_impl(file_path))
        {
          Log::warning("Failed to save updated settings to file: {}", file_path);
        }
      }

      auto loaded_settings_opt = ReflectionLibrary::from_generic_value<T>(loaded_settings);

      if (!loaded_settings_opt.has_value())
      {
        Log::error("Failed to parse loaded settings");
        return false;
      }

      settings_ = loaded_settings_opt.value();
      return true;
    }

    template<typename T>
    bool SettingsManager<T>::save_impl(const std::string& file_path)
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
            Log::error("Failed to create directory: {} - {}", directory, e.what());
            return false;
          }
        }
      }

      // Write settings to file
      std::ofstream file(file_path);
      if (!file.is_open())
      {
        Log::error("Failed to open settings file for writing: {}", file_path);
        return false;
      }
      std::string json_str = ReflectionLibrary::to_json(settings_, true);
      file << json_str;
      return true;
    }

    template<typename T>
    bool SettingsManager<T>::load(const std::string& file_path)
    {
      file_path_ = file_path;
      return load_impl(file_path);
    }

    template<typename T>
    bool SettingsManager<T>::save(const std::string& file_path)
    {
      std::string path = file_path.empty() ? file_path_ : file_path;

      if (path.empty())
      {
        Log::error("No file path specified for saving settings");
        return false;
      }

      if (!file_path.empty())
      {
        file_path_ = file_path;
      }

      return save_impl(path);
    }

    template<typename T>
    const T& SettingsManager<T>::get()
    {
      return settings_;
    }

    template<typename T>
    T& SettingsManager<T>::get_mutable()
    {
      return settings_;
    }

    template<typename T>
    void SettingsManager<T>::reset()
    {
      settings_ = T{};
    }

  }
}