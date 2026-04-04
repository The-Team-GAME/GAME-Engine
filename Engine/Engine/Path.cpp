//-----------------------------------------------------------------------------------
// file:    Path.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the Path class methods for cross-platform path
//          manipulation including normalization, conversion, and utility functions.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Path.h"
#include <algorithm>

namespace Engine {

  Path Path::normalize() const {
    std::string result = path_string;
    // Convert all backslashes to forward slashes
    std::replace(result.begin(), result.end(), '\\', '/');
    return Path(result);
  }

  Path Path::to_absolute() const {
    try {
      std::filesystem::path fs_path(path_string);
      std::filesystem::path abs_path = std::filesystem::absolute(fs_path);
      return Path(abs_path.string()).normalize();
    }
    catch (const std::filesystem::filesystem_error&) {
      // If conversion fails, return normalized path
      return normalize();
    }
  }

  Path Path::to_relative(const std::string& base_path) const {
    return to_relative(Path(base_path));
  }

  Path Path::to_relative(const Path& base_path) const {
    // Normalize both paths for comparison
    std::string normalized_full = normalize().path_string;
    std::string normalized_base = base_path.normalize().path_string;

    // Ensure base path ends with separator for proper matching
    if (!normalized_base.empty() && normalized_base.back() != '/') {
      normalized_base += '/';
    }

    // Check if full_path starts with base_path
    if (normalized_full.find(normalized_base) == 0) {
      // Return the substring after the base path
      return Path(normalized_full.substr(normalized_base.length()));
    }

    // If no match, try using std::filesystem::relative
    try {
      std::filesystem::path fs_full(path_string);
      std::filesystem::path fs_base(base_path.path_string);
      std::filesystem::path relative = std::filesystem::relative(fs_full, fs_base);
      return Path(relative.string()).normalize();
    }
    catch (const std::filesystem::filesystem_error&) {
      // If conversion fails, return normalized full path
      return Path(normalized_full);
    }
  }

  Path Path::ensure_trailing_separator() const {
    if (path_string.empty()) {
      return *this;
    }

    Path normalized = normalize();
    if (normalized.path_string.back() != '/') {
      return Path(normalized.path_string + '/');
    }
    return normalized;
  }

  Path Path::remove_trailing_separator() const {
    if (path_string.empty()) {
      return *this;
    }

    Path normalized = normalize();
    std::string result = normalized.path_string;
    while (!result.empty() && result.back() == '/') {
      result.pop_back();
    }
    return Path(result);
  }

  std::string Path::get_filename() const {
    std::string normalized = normalize().path_string;
    size_t pos = normalized.find_last_of('/');

    if (pos == std::string::npos) {
      return normalized; // No separator found, entire path is filename
    }

    return normalized.substr(pos + 1);
  }

  Engine::Path Path::get_directory() const {
    std::string normalized = normalize().path_string;
    size_t pos = normalized.find_last_of('/');

    if (pos == std::string::npos) {
      return Path(""); // No separator found, no directory
    }

    return Path(normalized.substr(0, pos));
  }

  std::string Path::get_extension() const {
    std::string filename = get_filename();
    size_t pos = filename.find_last_of('.');

    if (pos == std::string::npos || pos == 0) {
      return ""; // No extension or dot is at the beginning (.gitignore)
    }

    return filename.substr(pos);
  }

  Path Path::replace_extension(const std::string& new_extension) const {
    std::string normalized = normalize().path_string;
    size_t last_slash = normalized.find_last_of('/');
    size_t last_dot = normalized.find_last_of('.');

    // Only consider the dot if it's in the filename (after the last slash)
    if (last_dot == std::string::npos ||
      (last_slash != std::string::npos && last_dot < last_slash) ||
      last_dot == 0) {
      // No extension to replace, just append
      std::string result = normalized;
      if (!new_extension.empty()) {
        if (new_extension[0] != '.') {
          result += '.';
        }
        result += new_extension;
      }
      return Path(result);
    }

    // Remove old extension and add new one
    std::string result = normalized.substr(0, last_dot);
    if (!new_extension.empty()) {
      if (new_extension[0] != '.') {
        result += '.';
      }
      result += new_extension;
    }
    return Path(result);
  }

  bool Path::is_absolute() const {
    if (path_string.empty()) {
      return false;
    }

    // Check for Windows absolute path (e.g., "C:/..." or "C:\...")
    if (path_string.length() >= 2 && path_string[1] == ':') {
      return true;
    }

    // Check for Unix absolute path (starts with '/')
    if (path_string[0] == '/' || path_string[0] == '\\') {
      return true;
    }

    // Use std::filesystem as fallback
    try {
      std::filesystem::path fs_path(path_string);
      return fs_path.is_absolute();
    }
    catch (const std::filesystem::filesystem_error&) {
      return false;
    }
  }

  bool Path::exists() const {
    try {
      return std::filesystem::exists(path_string);
    }
    catch (const std::filesystem::filesystem_error&) {
      return false;
    }
  }

  std::string Path::to_string() const {
    return path_string;
  }

  const std::string& Path::string() const {
    return path_string;
  }

  Path::operator std::string() const {
    return path_string;
  }

  bool Path::operator==(const Path& other) const {
    return path_string == other.path_string;
  }

  bool Path::operator!=(const Path& other) const {
    return path_string != other.path_string;
  }

  bool Path::operator==(const std::string& other) const {
    return path_string == other;
  }

  bool Path::operator!=(const std::string& other) const {
    return path_string != other;
  }

  std::string Path::join_impl(const std::string& result) const {
    Path normalized(result);
    return normalized.normalize().path_string;
  }

} // namespace Engine

