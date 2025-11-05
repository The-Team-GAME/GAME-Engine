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
  - Supports method chaining for fluent API

  Where to use it:
  - Resource loading and serialization
  - File I/O operations
  - Any system that needs cross-platform path handling

  Example Usage:
  Path("Assets/images").join("player.png").to_string()
  Path("C:\\Users\\file.txt").normalize().get_filename()
  Path("Assets").to_absolute().ensure_trailing_separator()
  ==================================================================================*/
  class Path {
  private:
    std::string path_string;

  public:
    // Constructors
    Path() = default;
    Path(const std::string& path) : path_string(path) {}
    Path(const char* path) : path_string(path) {}
    Path(const Path& other) = default;
    Path(Path&& other) noexcept = default;

    // Assignment operators
    Path& operator=(const Path& other) = default;
    Path& operator=(Path&& other) noexcept = default;
    /*=================================================================================
    Function Brief:
    Normalizes the path by converting all backslashes to forward slashes.
    Returns a new Path object for method chaining.

    Example:
    Path("C:\\Users\\file.txt").normalize() -> Path with "C:/Users/file.txt"
    ==================================================================================*/
    Path normalize() const;

    /*=================================================================================
    Function Brief:
    Converts the path to an absolute path and normalizes it.
    Returns a new Path object for method chaining.

    Example:
    Path("Assets/image.png").to_absolute() -> Path with absolute path
    ==================================================================================*/
    Path to_absolute() const;

    /*=================================================================================
    Function Brief:
    Converts this absolute path to a relative path based on a base directory.
    Returns a new Path object for method chaining.

    Parameters:
    - base_path: The base directory path to make the path relative to

    Example:
    Path("C:/Projects/Game/Assets/image.png").to_relative("C:/Projects/Game/Assets/")
    -> Path with "image.png"
    ==================================================================================*/
    Path to_relative(const std::string& base_path) const;
    Path to_relative(const Path& base_path) const;

    /*=================================================================================
    Function Brief:
    Ensures the path ends with a path separator (/).
    Returns a new Path object for method chaining.

    Example:
    Path("Assets").ensure_trailing_separator() -> Path with "Assets/"
    ==================================================================================*/
    Path ensure_trailing_separator() const;

    /*=================================================================================
    Function Brief:
    Removes the trailing separator from the path if it exists.
    Returns a new Path object for method chaining.

    Example:
    Path("Assets/").remove_trailing_separator() -> Path with "Assets"
    ==================================================================================*/
    Path remove_trailing_separator() const;

    /*=================================================================================
    Function Brief:
    Joins path segments to this path.
    Returns a new Path object for method chaining.

    Example:
    Path("Assets").join("images", "player.png") -> Path with "Assets/images/player.png"
    ==================================================================================*/
    template<typename... Args>
    Path join(Args... segments) const;

    /*=================================================================================
    Function Brief:
    Gets the filename from the path (everything after the last separator).

    Example:
    Path("Assets/images/player.png").get_filename() -> "player.png"
    ==================================================================================*/
    std::string get_filename() const;

    /*=================================================================================
    Function Brief:
    Gets the directory portion of the path (everything before the last separator).

    Example:
    Path("Assets/images/player.png").get_directory() -> Path with "Assets/images"
    ==================================================================================*/
    Path get_directory() const;

    /*=================================================================================
    Function Brief:
    Gets the file extension from the path (including the dot).

    Example:
    Path("player.png").get_extension() -> ".png"
    ==================================================================================*/
    std::string get_extension() const;

    /*=================================================================================
    Function Brief:
    Checks if the path is absolute (has a root directory).

    Example:
    Path("C:/Users/file.txt").is_absolute() -> true (Windows)
    Path("Assets/file.txt").is_absolute() -> false
    ==================================================================================*/
    bool is_absolute() const;

    /*=================================================================================
    Function Brief:
    Checks if a file or directory exists at this path.
    ==================================================================================*/
    bool exists() const;

    /*=================================================================================
    Function Brief:
    Returns the underlying string representation of the path.
    ==================================================================================*/
    std::string to_string() const;
    const std::string& string() const;

    /*=================================================================================
    Function Brief:
    Implicit conversion to std::string for convenience.
    ==================================================================================*/
    operator std::string() const;

    /*=================================================================================
    Function Brief:
    Comparison operators
    ==================================================================================*/
    bool operator==(const Path& other) const;
    bool operator!=(const Path& other) const;
    bool operator==(const std::string& other) const;
    bool operator!=(const std::string& other) const;

  private:
    // Helper function for recursive join
    template<typename... Args>
    std::string join_impl(const std::string& result, const std::string& next, Args... rest) const;
    std::string join_impl(const std::string& result) const;
  };

  // Template implementations
  template<typename... Args>
  inline Path Path::join(Args... segments) const {
    return Path(join_impl(path_string, segments...));
  }

  template<typename... Args>
  inline std::string Path::join_impl(const std::string& result, const std::string& next, Args... rest) const {
    std::string combined = result;
    if (!combined.empty() && combined.back() != '/' && combined.back() != '\\') {
      combined += '/';
    }
    combined += next;
    return join_impl(combined, rest...);
  }

} // namespace Engine
