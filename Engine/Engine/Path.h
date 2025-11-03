//-----------------------------------------------------------------------------------
// file:    Path.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Path class for cross-platform path manipulation.
//          Provides utilities for normalizing paths, converting between absolute
//          and relative paths, and ensuring consistent path separators across platforms.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <filesystem>
#include <algorithm>

namespace Engine {

  /*=================================================================================
  Class Brief:
  Path - A cross-platform path utility class that handles path normalization,
  conversion between absolute/relative paths, and ensures consistent path separators.
  
  Key Features:
  - Normalizes path separators to forward slashes (/) for consistency
  - Converts between absolute and relative paths
  - Ensures paths end with separators when needed
  - Works seamlessly on Windows, Linux, and macOS
  
  Where to use it:
  - Resource loading and serialization
  - File I/O operations
  - Any system that needs cross-platform path handling
  ==================================================================================*/
  class Path {
  public:
    /*=================================================================================
    Function Brief:
    Normalizes a path by converting all backslashes to forward slashes.
    This ensures consistent path representation across platforms.

    Parameters:
    - path: The path string to normalize

    Returns:
    A string with all backslashes replaced by forward slashes.

    Example:
    "C:\\Users\\file.txt" -> "C:/Users/file.txt"
    ==================================================================================*/
    static std::string normalize(const std::string& path);

    /*=================================================================================
    Function Brief:
    Converts a path to an absolute path and normalizes it.

    Parameters:
    - path: The path to convert (can be relative or absolute)

    Returns:
    The absolute path with normalized separators.

    Example:
    "Assets/image.png" -> "C:/Projects/Game/Assets/image.png"
    ==================================================================================*/
    static std::string to_absolute(const std::string& path);

    /*=================================================================================
    Function Brief:
    Converts an absolute path to a relative path based on a base directory.

    Parameters:
    - full_path: The absolute path to convert
    - base_path: The base directory path to make the path relative to

    Returns:
    The relative path if full_path starts with base_path, otherwise returns full_path.

    Example:
    full_path = "C:/Projects/Game/Assets/image.png"
    base_path = "C:/Projects/Game/Assets/"
    returns   = "image.png"
    ==================================================================================*/
    static std::string to_relative(const std::string& full_path, const std::string& base_path);

    /*=================================================================================
    Function Brief:
    Ensures a path ends with a path separator (/).

    Parameters:
    - path: The path to process

    Returns:
    The path with a trailing separator if it didn't have one.

    Example:
    "Assets" -> "Assets/"
    "Assets/" -> "Assets/"
    ==================================================================================*/
    static std::string ensure_trailing_separator(const std::string& path);

    /*=================================================================================
    Function Brief:
    Removes the trailing separator from a path if it exists.

    Parameters:
    - path: The path to process

    Returns:
    The path without a trailing separator.

    Example:
    "Assets/" -> "Assets"
    "Assets" -> "Assets"
    ==================================================================================*/
    static std::string remove_trailing_separator(const std::string& path);

    /*=================================================================================
    Function Brief:
    Joins multiple path segments into a single path with proper separators.

    Parameters:
    - segments: Variable number of path segments to join

    Returns:
    A single path with segments joined by forward slashes.

    Example:
    join("Assets", "images", "player.png") -> "Assets/images/player.png"
    ==================================================================================*/
    template<typename... Args>
    static std::string join(const std::string& first, Args... rest);

    /*=================================================================================
    Function Brief:
    Gets the filename from a path (everything after the last separator).

    Parameters:
    - path: The path to extract the filename from

    Returns:
    The filename portion of the path.

    Example:
    "Assets/images/player.png" -> "player.png"
    ==================================================================================*/
    static std::string get_filename(const std::string& path);

    /*=================================================================================
    Function Brief:
    Gets the directory portion of a path (everything before the last separator).

    Parameters:
    - path: The path to extract the directory from

    Returns:
    The directory portion of the path.

    Example:
    "Assets/images/player.png" -> "Assets/images"
    ==================================================================================*/
    static std::string get_directory(const std::string& path);

    /*=================================================================================
    Function Brief:
    Gets the file extension from a path (including the dot).

    Parameters:
    - path: The path to extract the extension from

    Returns:
    The file extension including the dot, or empty string if no extension.

    Example:
    "player.png" -> ".png"
    "config" -> ""
    ==================================================================================*/
    static std::string get_extension(const std::string& path);

    /*=================================================================================
    Function Brief:
    Checks if a path is absolute (has a root directory).

    Parameters:
    - path: The path to check

    Returns:
    True if the path is absolute, false otherwise.

    Example:
    "C:/Users/file.txt" -> true (Windows)
    "/usr/bin/file" -> true (Unix)
    "Assets/file.txt" -> false
    ==================================================================================*/
    static bool is_absolute(const std::string& path);

    /*=================================================================================
    Function Brief:
    Checks if a file or directory exists at the given path.

    Parameters:
    - path: The path to check

    Returns:
    True if the path exists, false otherwise.
    ==================================================================================*/
    static bool exists(const std::string& path);

  private:
    // Helper function for recursive join
    static std::string join_impl(const std::string& result);

    template<typename... Args>
    static std::string join_impl(const std::string& result, const std::string& next, Args... rest);
  };

  // Template implementations
  template<typename... Args>
  inline std::string Path::join(const std::string& first, Args... rest) {
    return join_impl(first, rest...);
  }

  template<typename... Args>
  inline std::string Path::join_impl(const std::string& result, const std::string& next, Args... rest) {
    std::string combined = result;
    if (!combined.empty() && combined.back() != '/' && combined.back() != '\\') {
      combined += '/';
    }
    combined += next;
    return join_impl(combined, rest...);
  }

} // namespace Engine
