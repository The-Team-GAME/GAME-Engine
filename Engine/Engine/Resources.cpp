//-----------------------------------------------------------------------------------
// file:    Resources.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the resource management system for the game engine.
//          Handles loading, caching, and management of textures, fonts, sounds,
//          and prefabs. Provides resource deduplication, automatic cleanup of
//          unused resources, and support for drag-and-drop resource loading.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Resources.h"
#include <memory>
#include "Graphics/RendererInternal.h"
#include "Audio/AudioPlayerInternal.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <rapidjson/document.h>
#include "FileIO/PrefabsFileIO.h"
#include "FileIO/Helpers.h"
#include "Path.h"
#include "LevelManager.h"
#include "FileIO/LevelFileIO.h"
#include "LevelTabManager.h"
#include "Reflection/ReflectionLibrary.h"
#include "EventManager/EventManager.h"
#include "Events/EngineEvents.h"

// Define the actual storage for the extern variables
std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::TextureData>> Engine::Resources::texture_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::FontData>> Engine::Resources::font_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::Audio::SoundData>> Engine::Resources::sound_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::ECS::PrefabImpl>> Engine::Resources::prefab_cache;
std::unordered_map<std::string, Engine::EntityList> Engine::Resources::level_cache;

// Vector to hold shared_ptr references for persistent resources
std::vector<Engine::Resources::Resource> Engine::Resources::persistent_resources;

std::string Engine::Resources::resource_path = "";

bool Engine::Resources::init(const std::string& initial_resource_path)
{
  Log::info("Resource manager initialized");

  set_resource_path(initial_resource_path);

  return true;
}

bool Engine::Resources::exit()
{
  // Clear all caches
  texture_cache.clear();
  font_cache.clear();
  sound_cache.clear();
  prefab_cache.clear();
  level_cache.clear();

  // Clear persistent resources
  persistent_resources.clear();

  Log::info("Resource manager cleaned up");
  return true;
}

void Engine::Resources::set_resource_path(const std::string& path)
{
  // Convert to absolute path, normalize, and ensure trailing separator
  resource_path = Path(path).to_absolute().ensure_trailing_separator();

  Log::info("Resource path set to: {}", resource_path);
}

void Engine::Resources::load_all_assets()
{
  if (resource_path.empty())
  {
    Log::warning("Resource path is empty, cannot load all assets");
    return;
  }

  if (!std::filesystem::exists(resource_path))
  {
    Log::warning("Resource path does not exist: {}", resource_path);
    return;
  }

  Log::info("Loading all assets from: {}", resource_path);
  size_t loaded_count = 0;

  try
  {
    // Recursively iterate through all files in the resource path
    for (const auto& entry : std::filesystem::recursive_directory_iterator(resource_path))
    {
      if (entry.is_regular_file())
      {
        // Get relative path from resource_path
        std::string relative_path = Engine::Path(entry.path().string()).to_relative(resource_path);

        // Load the resource (persist=true to keep it loaded)
        Engine::Resources::Resource resource = Engine::Resources::load(relative_path, true, true);

        // Check if load was successful (not monostate)
        if (!std::holds_alternative<std::monostate>(resource))
        {
          loaded_count++;
        }
      }
    }

    Log::info("Finished loading all assets. Total loaded: {}", loaded_count);
  }
  catch (const std::filesystem::filesystem_error& e)
  {
    Log::error("Error while loading all assets: {}", e.what());
  }
}

void Engine::Resources::load_all_prefabs()
{
  if (resource_path.empty())
  {
    Log::warning("Resource path is empty, cannot load prefabs");
    return;
  }

  std::string prefabs_path = resource_path + "prefabs";
  if (!std::filesystem::exists(prefabs_path))
  {
    Log::warning("Prefabs path does not exist: {}", prefabs_path);
    return;
  }

  Log::info("Loading all prefabs from: {}", prefabs_path);
  size_t loaded_count = 0;

  try
  {
    // Iterate through all files in the prefabs folder
    for (const auto& entry : std::filesystem::recursive_directory_iterator(prefabs_path))
    {
      if (entry.is_regular_file())
      {
        std::string extension = entry.path().extension().string();
        // Only load .json files (prefabs)
        if (extension == ".json")
        {
          // Get relative path from resource_path
          std::string relative_path = Engine::Path(entry.path().string()).to_relative(resource_path);

          // Load the prefab (persist=true to keep it loaded)
          Engine::ECS::Prefab prefab = Engine::Resources::load_prefab(relative_path, true, true);

          // Check if load was successful (prefab is valid)
          if (prefab.is_valid())
          {
            loaded_count++;
          }
        }
      }
    }

    Log::info("Finished loading all prefabs. Total loaded: {}", loaded_count);
  }
  catch (const std::filesystem::filesystem_error& e)
  {
    Log::error("Error while loading prefabs: {}", e.what());
  }
}

std::string Engine::Resources::copy_to_resources(const std::string& filepath, const std::string& subfolder)
{
  // Normalize the input path
  std::string normalized_path = Engine::Path(filepath).normalize();

  // Check if file exists
  if (!std::filesystem::exists(normalized_path))
  {
    Log::warning("Cannot copy to resources - file does not exist: {}", normalized_path);
    return "";
  }

  // Get absolute resource path
  std::string abs_resource_path = Engine::Path(resource_path).to_absolute();
  std::string abs_file_path = Engine::Path(normalized_path).to_absolute();

  // Check if the file is already within the resource folder
  if (abs_file_path.find(abs_resource_path) == 0)
  {
    // File is already in resources, return relative path
    return Engine::Path(abs_file_path).to_relative(abs_resource_path);
  }

  // Extract just the filename (no directory path)
  std::string filename = Engine::Path(normalized_path).get_filename();

  // Construct destination path in resource root or subfolder
  std::string dest_dir = abs_resource_path;
  if (!subfolder.empty())
  {
    dest_dir = Engine::Path(abs_resource_path).join(subfolder);
    // Create subfolder if it doesn't exist
    if (!std::filesystem::exists(dest_dir))
    {
      try
      {
        std::filesystem::create_directories(dest_dir);
        Log::info("Created resource subfolder: {}", subfolder);
      }
      catch (const std::filesystem::filesystem_error& e)
      {
        Log::error("Failed to create resource subfolder: {}", e.what());
        return "";
      }
    }
  }

  std::string dest_path = Engine::Path(dest_dir).join(filename);

  // Check if a file with this name already exists in resources
  if (std::filesystem::exists(dest_path))
  {
    // File already exists - check if it's the same file
    if (std::filesystem::equivalent(abs_file_path, dest_path))
    {
      Log::info("File already exists in resources: {}", filename);
      return !subfolder.empty() ? Engine::Path(subfolder).join(filename) : Engine::Path(filename);
    }

    // Different file with same name - append number to make it unique
    std::string extension = Engine::Path(filename).get_extension();
    std::string base_name = filename;

    // Remove extension from base_name if it exists
    if (!extension.empty())
    {
      base_name = filename.substr(0, filename.length() - extension.length());
    }

    int counter = 1;

    do
    {
      filename = base_name + "_" + std::to_string(counter) + extension;
      dest_path = Engine::Path(dest_dir).join(filename);
      counter++;
    } while (std::filesystem::exists(dest_path));

    Log::info("File renamed to avoid conflict: {}", filename);
  }

  // Copy the file
  try
  {
    std::filesystem::copy_file(abs_file_path, dest_path);
    Log::info("Copied file to resources: {}", filename);
    return !subfolder.empty() ? Engine::Path(subfolder).join(filename) : Engine::Path(filename);
  }
  catch (const std::filesystem::filesystem_error& e)
  {
    Log::error("Failed to copy file to resources: {}", e.what());
    return "";
  }
}

std::string Engine::Resources::get_relative_path(const std::string& full_path)
{
  // Special handling for unit_rect texture
  if (full_path == Engine::Graphics::get_rect_texture()->path)
  {
    return "UNIT_RECT";
  }

  // Use the Path class to convert to relative path
  return Engine::Path(full_path).to_relative(resource_path);
}

Engine::Graphics::Texture Engine::Resources::load_texture(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Special handling for unit_rect texture
  if (filepath == Engine::Graphics::get_rect_texture()->path)
  {
    return Engine::Graphics::get_rect_texture();
  }

  // Construct full path using Path class
  std::string full_path = append_resource_path ? Engine::Path(resource_path).join(filepath) : Engine::Path(filepath).normalize();

  // If loading from outside resources and not appending resource path, copy to resources
  std::string actual_filepath = filepath;
  if (!append_resource_path)
  {
    std::string copied_path = Engine::Resources::copy_to_resources(filepath, "images");
    if (!copied_path.empty())
    {
      actual_filepath = copied_path;
      full_path = Engine::Path(resource_path).join(actual_filepath);
    }
  }

  // Check if texture is already in cache (use full path for cache key)
  auto it = texture_cache.find(full_path);
  if (it != texture_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      // Log::info("Texture loaded from cache: " + full_path);
      return cached;
    }
    else
    {
      // Expired, remove from cache
      texture_cache.erase(it);
    }
  }

  // Load new texture
  auto texture = std::make_shared<Engine::Graphics::TextureData>(full_path);

  // Only cache if the texture was successfully loaded
  if (texture->id != 0)
  {
    texture_cache[full_path] = texture;
    if (persist)
    {
      persistent_resources.push_back(texture);
      Log::info("Texture loaded (persistent): {}", full_path);
    }
    else
    {
      Log::info("Texture loaded: {}", full_path);
    }
  }
  else
  {
    Log::warning("Failed to load texture: {}", full_path);
  }

  return texture;
}

Engine::Graphics::Font Engine::Resources::load_font(const std::string& filepath, u32 size, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Engine::Path(resource_path).join(filepath) : Engine::Path(filepath).normalize();

  // If loading from outside resources and not appending resource path, copy to resources
  std::string actual_filepath = filepath;
  if (!append_resource_path)
  {
    std::string copied_path = Engine::Resources::copy_to_resources(filepath, "fonts");
    if (!copied_path.empty())
    {
      actual_filepath = copied_path;
      full_path = Engine::Path(resource_path).join(actual_filepath);
    }
  }

  // Create a unique key combining filepath and size
  std::string cache_key = full_path + "_" + std::to_string(size);

  // Check if font is already in cache
  auto it = font_cache.find(cache_key);
  if (it != font_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      // Log::info("Font loaded from cache: " + full_path + " (size: " + std::to_string(size) + ")");
      return cached;
    }
    else
    {
      // Expired, remove from cache
      font_cache.erase(it);
    }
  }

  // Load new font
  auto font = std::make_shared<Engine::Graphics::FontData>(full_path, size);

  // Cache the font
  font_cache[cache_key] = font;
  if (persist)
  {
    persistent_resources.push_back(font);
    Log::info("Font loaded (persistent): {} (size: {})", full_path, size);
  }
  else
  {
    Log::info("Font loaded: {} (size: {})", full_path, size);
  }

  return font;
}

Engine::Audio::Sound Engine::Resources::load_sound(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Engine::Path(resource_path).join(filepath) : Engine::Path(filepath).normalize();

  // If loading from outside resources and not appending resource path, copy to resources
  std::string actual_filepath = filepath;
  if (!append_resource_path)
  {
    std::string copied_path = Engine::Resources::copy_to_resources(filepath, "sounds");
    if (!copied_path.empty())
    {
      actual_filepath = copied_path;
      full_path = Engine::Path(resource_path).join(actual_filepath);
    }
  }

  // Check if sound is already in cache
  auto it = sound_cache.find(full_path);
  if (it != sound_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      // Log::info("Sound loaded from cache: " + full_path);
      return cached;
    }
    else
    {
      // Expired, remove from cache
      sound_cache.erase(it);
    }
  }

  // Load new sound (not streamed)
  auto sound = std::make_shared<Engine::Audio::SoundData>(full_path, false);

  // Only cache if the sound was successfully loaded
  if (sound->sound != nullptr)
  {
    sound_cache[full_path] = sound;
    if (persist)
    {
      persistent_resources.push_back(sound);
      Log::info("Sound loaded (persistent): {}", full_path);
    }
    else
    {
      Log::info("Sound loaded: {}", full_path);
    }
  }
  else
  {
    Log::warning("Failed to load sound: {}", full_path);
  }

  return sound;
}

Engine::Audio::Sound Engine::Resources::load_music(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Engine::Path(resource_path).join(filepath) : Engine::Path(filepath).normalize();

  // If loading from outside resources and not appending resource path, copy to resources
  std::string actual_filepath = filepath;
  if (!append_resource_path)
  {
    std::string copied_path = Engine::Resources::copy_to_resources(filepath, "sounds");
    if (!copied_path.empty())
    {
      actual_filepath = copied_path;
      full_path = Engine::Path(resource_path).join(actual_filepath);
    }
  }

  // Check if music is already in cache
  auto it = sound_cache.find(full_path);
  if (it != sound_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      // Log::info("Music loaded from cache: " + full_path);
      return cached;
    }
    else
    {
      // Expired, remove from cache
      sound_cache.erase(it);
    }
  }

  // Load new music (streamed from disk)
  auto music = std::make_shared<Engine::Audio::SoundData>(full_path, true);

  // Only cache if the music was successfully loaded
  if (music->sound != nullptr)
  {
    sound_cache[full_path] = music;
    if (persist)
    {
      persistent_resources.push_back(music);
      Log::info("Music loaded (persistent): {}", full_path);
    }
    else
    {
      Log::info("Music loaded: {}", full_path);
    }
  }
  else
  {
    Log::warning("Failed to load music: {}", full_path);
  }

  return music;
}

Engine::Resources::Resource Engine::Resources::load(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Path(resource_path).join(filepath) : Path(filepath).normalize();

  // Extract file extension using Path class
  std::string extension = Path(filepath).get_extension();
  if (extension.empty())
  {
    Log::warning("No file extension found for: {}", filepath);
    return std::monostate{};
  }

  // Remove leading dot and convert to lowercase
  if (extension[0] == '.') {
    extension = extension.substr(1);
  }
  std::transform(extension.begin(), extension.end(), extension.begin(),
    [](u8 c) { return static_cast<u8>(std::tolower(c)); });

  // Image extensions (textures)
  if (extension == "png" || extension == "jpg" || extension == "jpeg" ||
    extension == "bmp" || extension == "tga" || extension == "gif")
  {
    return load_texture(filepath, append_resource_path, persist);
  }

  // Font extensions
  if (extension == "ttf" || extension == "otf")
  {
    return load_font(filepath, 32, append_resource_path, persist); // Default size 32
  }

  // Audio extensions
  if (extension == "wav" || extension == "mp3" || extension == "ogg" || extension == "flac")
  {
    // Check file size to determine if it should be streamed
    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
      std::streamsize size = file.tellg();
      file.close();

      // Files larger than 1MB are loaded as music (streamed)
      if (size > 1024 * 1024)
      {
        return load_music(filepath, append_resource_path, persist);
      }
    }
    return load_sound(filepath, append_resource_path, persist);
  }

  // JSON extensions - could be prefabs
  if (extension == "json")
  {
    // Read the file to check its dataType
    std::string json_content = Engine::FileIO::read_string_from_file(full_path).value_or("");

    if (json_content.empty())
    {
      Log::warning("Failed to read JSON file: {}", full_path);
      return std::monostate{};
    }

    std::optional<ReflectionLibrary::GenericValue> value_opt = ReflectionLibrary::generic_value_from_json(json_content);

    if (!value_opt.has_value() || !value_opt->isObject())
    {
      Log::warning("Invalid JSON format in file: {}", full_path);
      return std::monostate{};
    }

    ReflectionLibrary::GenericValue& value = value_opt.value();

    if (value.has("dataType") && value["dataType"].isString())
    {
      std::string data_type = value["dataType"].getString();

      // Check if it's an Entity (prefab)
      if (data_type == "Prefab")
      {
        return load_prefab(filepath, append_resource_path, persist);
      }

      // Check if it's a Level
      if (data_type == "Level")
      {
        return load_level(filepath, append_resource_path, persist);
      }
    }

    Log::warning("Unknown JSON dataType in file: {}", full_path);
    return std::monostate{};
  }

  Log::warning("Unknown file extension: {} for file: {}", extension, filepath);
  return std::monostate{};
}

Engine::Graphics::Texture Engine::Resources::get_rect_texture()
{
  return Engine::Graphics::get_rect_texture();
}

Engine::ECS::Prefab Engine::Resources::load_prefab(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Engine::Path(resource_path).join(filepath) : Engine::Path(filepath).normalize();

  // If loading from outside resources and not appending resource path, copy to resources
  std::string actual_filepath = filepath;
  if (!append_resource_path)
  {
    std::string copied_path = Engine::Resources::copy_to_resources(filepath, "prefabs");
    if (!copied_path.empty())
    {
      actual_filepath = copied_path;
      full_path = Engine::Path(resource_path).join(actual_filepath);
    }
  }

  // Check if prefab is already in cache
  auto it = prefab_cache.find(full_path);
  if (it != prefab_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      // Log::info("Prefab loaded from cache: " + full_path);
      return Engine::ECS::Prefab(cached); // Wrap in handle
    }
    else
    {
      // Expired, remove from cache
      prefab_cache.erase(it);
    }
  }

  // Deserialize prefab
  Engine::ECS::Prefab prefab = Engine::FileIO::deserialize_prefab_from_json_file(full_path);

  if (!prefab.is_valid())
  {
    Log::warning("Failed to deserialize prefab: {}", full_path);
    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "Failed to load prefab: " + actual_filepath,
      Engine::Events::ToastType::Error,
      5.0f
    );
    return Engine::ECS::Prefab(); // Return empty prefab handle
  }

  // Check for loading errors (corrupted data, missing components, etc.)
  if (prefab.get_impl()->has_load_errors())
  {
    const auto& errors = prefab.get_impl()->get_load_errors();
    std::string error_summary = "Prefab '" + actual_filepath + "' loaded with errors:\n";
    for (size_t i = 0; i < errors.size() && i < 3; ++i) // Show first 3 errors
    {
      error_summary += "- " + errors[i] + "\n";
    }
    if (errors.size() > 3)
    {
      error_summary += "... and " + std::to_string(errors.size() - 3) + " more errors";
    }
    Log::warning("{}", error_summary);
    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      error_summary,
      Engine::Events::ToastType::Warning,
      8.0f
    );
  }

  // Set the filepath in the prefab
  prefab.get_impl()->set_prefab_filepath(actual_filepath);

  // Cache the prefab implementation
  prefab_cache[full_path] = prefab.get_impl();
  if (persist)
  {
    persistent_resources.push_back(prefab);
    Log::info("Prefab loaded (persistent): {}", full_path);
  }
  else
  {
    Log::info("Prefab loaded: {}", full_path);
  }

  return prefab;
}

bool Engine::Resources::save_prefab(const std::string& filepath, const Engine::ECS::Prefab& prefab, bool append_resource_path)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Path(resource_path).join(filepath) : Path(filepath).normalize();

  if (!Engine::FileIO::serialize_prefab_to_json_file(full_path, prefab))
  {
    return false;
  }

  // Update cache with the implementation
  prefab_cache[full_path] = prefab.get_impl();

  return true;
}

void Engine::Resources::cache_prefab(const std::string& filepath, const Engine::ECS::Prefab& prefab, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Path(resource_path).join(filepath) : Path(filepath).normalize();

  // Ensure the prefab impl has its filepath set
  if (prefab.get_impl() && prefab.get_impl()->get_prefab_filepath().empty()) {
    prefab.get_impl()->set_prefab_filepath(filepath);
  }

  // Add to cache without saving to disk
  prefab_cache[full_path] = prefab.get_impl();

  if (persist)
  {
    persistent_resources.push_back(prefab);
    Log::info("Level loaded (persistent): {}", full_path);
  }

  Log::info("Prefab cached: {}", full_path);
}

Engine::EntityList Engine::Resources::load_level(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Path(resource_path).join(filepath) : Path(filepath).normalize();

  // If loading from outside resources and not appending resource path, copy to resources
  std::string actual_filepath = filepath;
  if (!append_resource_path)
  {
    std::string copied_path = Engine::Resources::copy_to_resources(filepath, "levels");
    if (!copied_path.empty())
    {
      actual_filepath = copied_path;
      full_path = Engine::Path(resource_path).join(actual_filepath);
    }
  }

  // Check if level is already in cache
  auto it = level_cache.find(full_path);
  if (it != level_cache.end())
  {
    // Log::info("Level loaded from cache: " + full_path);
    return it->second;
  }

  // Deserialize level
  Engine::EntityList entities = Engine::FileIO::deserialize_level_from_json_file(full_path);

  // Cache the level
  level_cache[full_path] = entities;
  if (persist)
  {
    persistent_resources.push_back(entities);
    Log::info("Level loaded (persistent): {}", full_path);
  }
  else
  {
    Log::info("Level loaded: {}", full_path);
  }

  return entities;
}

bool Engine::Resources::save_level(const std::string& filepath, const Engine::EntityList& entities, bool append_resource_path)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Path(resource_path).join(filepath) : Path(filepath).normalize();

  Log::info("Saving level to: {}", full_path);

  if (!Engine::FileIO::serialize_level_to_json_file(full_path, entities))
  {
    return false;
  }

  // Update cache with the level
  level_cache[full_path] = entities;

  return true;
}

void Engine::Resources::cache_level(const std::string& filepath, const Engine::EntityList& entities, bool append_resource_path, bool persist)
{
  // Construct full path using Path class
  std::string full_path = append_resource_path ? Path(resource_path).join(filepath) : Path(filepath).normalize();

  // Add to cache without saving to disk
  level_cache[full_path] = entities;

  if (persist)
  {
    persistent_resources.push_back(entities);
    Log::info("Level cached (persistent): {}", full_path);
  }

  Log::info("Level cached: {}", full_path);
}

bool Engine::Resources::open_level(const std::string& filepath, bool append_resource_path)
{
#ifndef GAME_BUILD
  // In editor mode, use the tab manager for level opening
  return LevelTabManager::open_tab(filepath, append_resource_path);
#else
  // In game mode or for additive loading, use LevelManager
  std::string level_id = LevelManager::open_level(filepath, append_resource_path);
  return !level_id.empty();
#endif
}

bool Engine::Resources::open(const std::string& filepath, bool append_resource_path)
{
  std::string full_path = append_resource_path ? Path(resource_path).join(filepath) : Path(filepath).normalize();

  // Get file extension
  std::string extension = Path(filepath).get_extension();
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

  // For JSON files, we need to check the dataType
  if (extension == ".json") {
    // Read the file to check dataType
    auto json_str = Engine::FileIO::read_string_from_file(full_path);

    if (!json_str.has_value()) {
      Log::warning("Failed to read file for opening: {}", filepath);
      return false;
    }

    // Parse JSON to check dataType
    std::optional<ReflectionLibrary::GenericValue> value_opt = ReflectionLibrary::generic_value_from_json(json_str.value());

    if (!value_opt) {
      Log::warning("Failed to parse JSON file: {}", filepath);
      return false;
    }

    ReflectionLibrary::GenericValue value = value_opt.value();

    // Check if it has a dataType field
    if (value.has("dataType")) {
      std::string data_type = value["dataType"].getString();

      if (data_type == "Level") {
        return open_level(filepath, append_resource_path);
      }
      else {
        Log::warning("Unknown dataType in JSON file: {}", data_type);
        Engine::EventManager::send_event<Engine::Events::ToastEvent>(
          "Cannot open file: Unknown JSON data type",
          Engine::Events::ToastType::Error,
          5.0f
        );
        return false;
      }
    }
    else {
      Log::warning("JSON file missing dataType field: {}", filepath);
      return false;
    }
  }

  Log::warning("Unsupported file type for opening: {}", extension);
  Engine::EventManager::send_event<Engine::Events::ToastEvent>(
    "Cannot open file: Unsupported file type",
    Engine::Events::ToastType::Error,
    5.0f
  );
  return false;
}


void Engine::Resources::remove_from_cache(const std::string& filepath, bool append_resource_path)
{
  std::string path = append_resource_path ?
    Path(resource_path).join(filepath).to_string() : filepath;

  // Remove from level cache
  auto level_it = level_cache.find(path);
  if (level_it != level_cache.end()) {
    Log::info("Removed level from cache: {}", path);
    level_cache.erase(level_it);

    // Check if this was the current level and clear it
    if (LevelManager::get_current_level_path() == path) {
      LevelManager::set_current_level_path("");
      Log::warning("Deleted currently open level - current level path cleared");
    }
  }

  // Remove from texture cache
  auto texture_it = texture_cache.find(path);
  if (texture_it != texture_cache.end()) {
    Log::info("Removed texture from cache: {}", path);
    texture_cache.erase(texture_it);
  }

  // Remove from font cache
  auto font_it = font_cache.find(path);
  if (font_it != font_cache.end()) {
    Log::info("Removed font from cache: {}", path);
    font_cache.erase(font_it);
  }

  // Remove from sound cache
  auto sound_it = sound_cache.find(path);
  if (sound_it != sound_cache.end()) {
    Log::info("Removed sound from cache: {}", path);
    sound_cache.erase(sound_it);
  }

  // Remove from prefab cache
  auto prefab_it = prefab_cache.find(path);
  if (prefab_it != prefab_cache.end()) {
    Log::info("Removed prefab from cache: {}", path);
    prefab_cache.erase(prefab_it);
  }

  // Note: persistent_resources vector doesn't store filepath info,
  // so we rely on shared_ptr reference counting to clean up when caches are cleared
}


void Engine::Resources::clear_unused_resources()
{
  // Clear expired texture cache entries
  for (auto it = texture_cache.begin(); it != texture_cache.end();)
  {
    if (it->second.expired())
    {
      Log::info("Clearing unused texture from cache: {}", it->first);
      it = texture_cache.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // Clear expired font cache entries
  for (auto it = font_cache.begin(); it != font_cache.end();)
  {
    if (it->second.expired())
    {
      Log::info("Clearing unused font from cache: {}", it->first);
      it = font_cache.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // Clear expired sound cache entries
  for (auto it = sound_cache.begin(); it != sound_cache.end();)
  {
    if (it->second.expired())
    {
      Log::info("Clearing unused sound from cache: {}", it->first);
      it = sound_cache.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // Clear expired prefab cache entries
  for (auto it = prefab_cache.begin(); it != prefab_cache.end();)
  {
    if (it->second.expired())
    {
      Log::info("Clearing unused prefab from cache: {}", it->first);
      it = prefab_cache.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

Engine::Resources::ResourceStats Engine::Resources::get_resource_stats()
{
  ResourceStats stats = {};

  for (const auto& pair : texture_cache)
  {
    stats.texture_count++;
    if (!pair.second.expired())
    {
      stats.texture_alive++;
    }
  }

  for (const auto& pair : font_cache)
  {
    stats.font_count++;
    if (!pair.second.expired())
    {
      stats.font_alive++;
    }
  }

  for (const auto& pair : sound_cache)
  {
    stats.sound_count++;
    if (!pair.second.expired())
    {
      stats.sound_alive++;
    }
  }

  for (const auto& pair : prefab_cache)
  {
    stats.prefab_count++;
    if (!pair.second.expired())
    {
      stats.prefab_alive++;
    }
  }

  return stats;
}

Engine::Graphics::PostProcessing::Settings Engine::Resources::load_postprocessing_settings(const std::string& filepath, bool append_resource_path)
{
  std::string full_path = append_resource_path ? (resource_path + filepath) : filepath;
  auto opt = Engine::FileIO::read_string_from_file(full_path);
  if (!opt.has_value()) {
    Log::error("Resources: Failed to read post-processing preset: {}", full_path);
    return Engine::Graphics::PostProcessing::Settings{};
  }
  return Engine::Graphics::PostProcessing::deserialize_settings_from_json(opt.value());
}

bool Engine::Resources::save_postprocessing_settings(const std::string& filepath, const Engine::Graphics::PostProcessing::Settings& settings, bool append_resource_path)
{
  std::string full_path = append_resource_path ? (resource_path + filepath) : filepath;

  // Ensure directory exists
  std::filesystem::path p(full_path);
  if (p.has_parent_path()) {
    std::filesystem::create_directories(p.parent_path());
  }

  std::string json = Engine::Graphics::PostProcessing::serialize_settings_to_json(settings);
  bool result = Engine::FileIO::write_string_to_file(full_path, json);
  if (result) {
    Log::info("Resources: Saved post-processing preset to {}", full_path);
  }
  else {
    Log::error("Resources: Failed to save post-processing preset to {}", full_path);
  }
  return result;
}