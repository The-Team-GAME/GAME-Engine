//-----------------------------------------------------------------------------------
// file:    Helpers.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements helper functions for file I/O operations including
//          reading and writing strings to files with automatic directory creation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Helpers.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "../Logger.h"

namespace Engine
{
  namespace FileIO
  {
    std::optional<std::string> read_string_from_file(const std::string& file_path)
    {
      std::ifstream file(file_path);

      if (!file.is_open())
      {
        Log::warning("Failed to open file for reading: {}", file_path);
        return std::nullopt;
      }

      std::stringstream buffer;
      buffer << file.rdbuf();
      file.close();

      return buffer.str();
    }

    bool write_string_to_file(const std::string& file_path, const std::string& content)
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
            Log::info("Created directory: {}", directory);
          }
          catch (const std::filesystem::filesystem_error& e)
          {
            Log::error("Failed to create directory: {} - {}", directory, e.what());
            return false;
          }
        }
      }

      // Write content to file
      std::ofstream file(file_path);
      if (!file.is_open())
      {
        Log::error("Failed to open file for writing: {}", file_path);
        return false;
      }

      file << content;
      file.close();

      Log::info("Successfully wrote to file: {}", file_path);
      return true;
    }
  }
}

