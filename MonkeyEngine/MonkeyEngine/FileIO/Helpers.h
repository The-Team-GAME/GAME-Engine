//-----------------------------------------------------------------------------------
// file:    Helpers.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares helper functions for file I/O operations including
//          reading and writing strings to files with automatic directory creation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <optional>

namespace Engine
{
  namespace FileIO
  {
    /*=================================================================================
    Function Brief:
    Reads the entire contents of a file into a string.

    Parameters:
    file_path - Path to the file to read.

    Returns:
    std::optional<std::string> containing the file contents if successful,
    std::nullopt if the file couldn't be opened or read.

    Where to call it:
    When you need to load text data from a file (e.g., configuration, scripts, logs).
    ==================================================================================*/
    std::optional<std::string> read_string_from_file(const std::string& file_path);

    /*=================================================================================
    Function Brief:
    Writes a string to a file, automatically creating parent directories if they
    don't exist.

    Parameters:
    file_path - Path to the file to write.
    content   - The string content to write to the file.

    Returns:
    True if the file was written successfully, false on error.

    Where to call it:
    When you need to save text data to a file with automatic directory creation
    (e.g., saving logs, configuration, or generated content).
    ==================================================================================*/
    bool write_string_to_file(const std::string& file_path, const std::string& content);
  }
}
