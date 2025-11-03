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

  std::string Path::normalize(const std::string& path) {
    std::string result = path;
    // Convert all backslashes to forward slashes
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
  }

  std::string Path::to_absolute(const std::string& path) {
    try {
      std::filesystem::path fs_path(path);
      std::filesystem::path abs_path = std::filesystem::absolute(fs_path);
      return normalize(abs_path.string());
    }
    catch (const std::filesystem::filesystem_error&) {
      // If conversion fails, return normalized original path
      return normalize(path);
    }
  }

  std::string Path::to_relative(const std::string& full_path, const std::string& base_path) {
    // Normalize both paths for comparison
    std::string normalized_full = normalize(full_path);
    std::string normalized_base = normalize(base_path);

    // Ensure base path ends with separator for proper matching
    if (!normalized_base.empty() && normalized_base.back() != '/') {
      normalized_base += '/';
    }

    // Check if full_path starts with base_path
    if (normalized_full.find(normalized_base) == 0) {
      // Return the substring after the base path
      return normalized_full.substr(normalized_base.length());
    }

    // If no match, try using std::filesystem::relative
    try {
      std::filesystem::path fs_full(full_path);
      std::filesystem::path fs_base(base_path);
      std::filesystem::path relative = std::filesystem::relative(fs_full, fs_base);
      return normalize(relative.string());
    }
    catch (const std::filesystem::filesystem_error&) {
      // If conversion fails, return normalized full path
      return normalized_full;
    }
  }

  std::string Path::ensure_trailing_separator(const std::string& path) {
    if (path.empty()) {
      return path;
    }

    std::string result = normalize(path);
    if (result.back() != '/') {
      result += '/';
    }
    return result;
  }

  std::string Path::remove_trailing_separator(const std::string& path) {
    if (path.empty()) {
      return path;
    }

    std::string result = normalize(path);
    while (!result.empty() && result.back() == '/') {
      result.pop_back();
    }
    return result;
  }

  std::string Path::get_filename(const std::string& path) {
    std::string normalized = normalize(path);
    size_t pos = normalized.find_last_of('/');
    
    if (pos == std::string::npos) {
      return normalized; // No separator found, entire path is filename
    }
    
    return normalized.substr(pos + 1);
  }

  std::string Path::get_directory(const std::string& path) {
    std::string normalized = normalize(path);
    size_t pos = normalized.find_last_of('/');
    
    if (pos == std::string::npos) {
      return ""; // No separator found, no directory
    }
    
    return normalized.substr(0, pos);
  }

  std::string Path::get_extension(const std::string& path) {
    std::string filename = get_filename(path);
    size_t pos = filename.find_last_of('.');
    
    if (pos == std::string::npos || pos == 0) {
      return ""; // No extension or dot is at the beginning (.gitignore)
    }
    
    return filename.substr(pos);
  }

  bool Path::is_absolute(const std::string& path) {
    if (path.empty()) {
      return false;
    }

    // Check for Windows absolute path (e.g., "C:/..." or "C:\...")
    if (path.length() >= 2 && path[1] == ':') {
      return true;
    }

    // Check for Unix absolute path (starts with '/')
    if (path[0] == '/' || path[0] == '\\') {
      return true;
    }

    // Use std::filesystem as fallback
    try {
      std::filesystem::path fs_path(path);
      return fs_path.is_absolute();
    }
    catch (const std::filesystem::filesystem_error&) {
      return false;
    }
  }

  bool Path::exists(const std::string& path) {
    try {
      return std::filesystem::exists(path);
    }
    catch (const std::filesystem::filesystem_error&) {
      return false;
    }
  }

  std::string Path::join_impl(const std::string& result) {
    return normalize(result);
  }

} // namespace Engine
