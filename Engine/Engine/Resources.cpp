#include "Resources.h"
#include <memory>
#include "Graphics/RendererInternal.h"
#include "Audio/AudioPlayerInternal.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <rapidjson/document.h>
#include "FileIO/PrefabsFileIO.h"

// Define the actual storage for the extern variables
std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::TextureData>> Engine::Resources::texture_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::FontData>> Engine::Resources::font_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::Audio::SoundData>> Engine::Resources::sound_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::ECS::PrefabImpl>> Engine::Resources::prefab_cache;

// Vector to hold shared_ptr references for persistent resources
std::vector<Engine::Resources::Resource> Engine::Resources::persistent_resources;

Engine::Graphics::Texture Engine::Resources::unit_rect;
std::string Engine::Resources::resource_path = "";

bool Engine::Resources::init()
{
  // Create a 1x1 white pixel texture for unit_rect
  unsigned char white_pixel[4] = { 255, 255, 255, 255 };

  auto texture_data = std::make_shared<Engine::Graphics::TextureData>();
  texture_data->width = 1;
  texture_data->height = 1;

  glGenTextures(1, &texture_data->id);
  glBindTexture(GL_TEXTURE_2D, texture_data->id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);

  unit_rect = texture_data;

  Log::info("Resource manager initialized");
  return true;
}

bool Engine::Resources::exit()
{
  // Clear all caches
  texture_cache.clear();
  font_cache.clear();
  sound_cache.clear();
  prefab_cache.clear();

  // Clear persistent resources
  persistent_resources.clear();

  // unit_rect will be automatically cleaned up when it goes out of scope
  unit_rect.reset();

  Log::info("Resource manager cleaned up");
  return true;
}

void Engine::Resources::set_resource_path(const std::string& path)
{
  resource_path = path;

  // Ensure the path ends with a separator
  if (!resource_path.empty() && resource_path.back() != '/' && resource_path.back() != '\\')
  {
    resource_path += '/';
  }

  Log::info("Resource path set to: " + resource_path);
}

std::string Engine::Resources::get_relative_path(const std::string& full_path)
{
  if (full_path.find(resource_path) == 0)
  {
    return full_path.substr(resource_path.length());
  }
  return full_path;
}

Engine::Graphics::Texture Engine::Resources::load_texture(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path
  std::string full_path = append_resource_path ? resource_path + filepath : filepath;

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
      Log::info("Texture loaded (persistent): " + full_path);
    }
    else
    {
      Log::info("Texture loaded: " + full_path);
    }
  }
  else
  {
    Log::warning("Failed to load texture: " + full_path);
  }

  return texture;
}

Engine::Graphics::Font Engine::Resources::load_font(const std::string& filepath, u32 size, bool append_resource_path, bool persist)
{
  // Construct full path
  std::string full_path = append_resource_path ? resource_path + filepath : filepath;

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
    Log::info("Font loaded (persistent): " + full_path + " (size: " + std::to_string(size) + ")");
  }
  else
  {
    Log::info("Font loaded: " + full_path + " (size: " + std::to_string(size) + ")");
  }

  return font;
}

Engine::Audio::Sound Engine::Resources::load_sound(const std::string& filepath, bool append_resource_path, bool persist)
{
  std::string full_path = append_resource_path ? resource_path + filepath : filepath;

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
      Log::info("Sound loaded (persistent): " + full_path);
    }
    else
    {
      Log::info("Sound loaded: " + full_path);
    }
  }
  else
  {
    Log::warning("Failed to load sound: " + full_path);
  }

  return sound;
}

Engine::Audio::Sound Engine::Resources::load_music(const std::string& filepath, bool append_resource_path, bool persist)
{
  std::string full_path = append_resource_path ? resource_path + filepath : filepath;

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
      Log::info("Music loaded (persistent): " + full_path);
    }
    else
    {
      Log::info("Music loaded: " + full_path);
    }
  }
  else
  {
    Log::warning("Failed to load music: " + full_path);
  }

  return music;
}

Engine::Resources::Resource Engine::Resources::load(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path
  std::string full_path = append_resource_path ? resource_path + filepath : filepath;

  // Extract file extension
  size_t dot_pos = filepath.find_last_of('.');
  if (dot_pos == std::string::npos)
  {
    Log::warning("No file extension found for: " + filepath);
    return std::monostate{};
  }

  std::string extension = filepath.substr(dot_pos + 1);
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
    std::ifstream file(full_path);
    if (!file.is_open())
    {
      Log::warning("Failed to open JSON file: " + full_path);
      return std::monostate{};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_content = buffer.str();
    file.close();

    // Parse JSON to check dataType
    rapidjson::Document doc;
    doc.Parse(json_content.c_str());

    if (doc.HasParseError())
    {
      Log::warning("Failed to parse JSON file: " + full_path);
      return std::monostate{};
    }

    // Check if it's an Entity (prefab)
    if (doc.HasMember("dataType") && doc["dataType"].IsString() &&
      std::string(doc["dataType"].GetString()) == "Entity")
    {
      return load_prefab(filepath, append_resource_path, persist);
    }

    Log::warning("Unknown JSON dataType in file: " + full_path);
    return std::monostate{};
  }

  Log::warning("Unknown file extension: " + extension + " for file: " + filepath);
  return std::monostate{};
}

Engine::Graphics::Texture Engine::Resources::get_rect_texture()
{
  return unit_rect;
}

Engine::ECS::Prefab Engine::Resources::load_prefab(const std::string& filepath, bool append_resource_path, bool persist)
{
  // Construct full path
  std::string full_path = append_resource_path ? resource_path + filepath : filepath;

  // Check if prefab is already in cache
  auto it = prefab_cache.find(full_path);
  if (it != prefab_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      // Log::info("Prefab loaded from cache: " + full_path);
      return cached;
    }
    else
    {
      // Expired, remove from cache
      prefab_cache.erase(it);
    }
  }

  // Load prefab from file
  std::ifstream file(full_path);
  if (!file.is_open())
  {
    Log::warning("Failed to open prefab file: " + full_path);
    return nullptr;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string json_content = buffer.str();
  file.close();

  // Deserialize prefab
  Engine::ECS::Prefab prefab = Engine::FileIO::deserialize_prefab_from_json(json_content);

  if (!prefab)
  {
    Log::warning("Failed to deserialize prefab: " + full_path);
    return nullptr;
  }

  // Cache the prefab
  prefab_cache[full_path] = prefab;
  if (persist)
  {
    persistent_resources.push_back(prefab);
    Log::info("Prefab loaded (persistent): " + full_path);
  }
  else
  {
    Log::info("Prefab loaded: " + full_path);
  }

  return prefab;
}

bool Engine::Resources::save_prefab(const std::string& filepath, const Engine::ECS::Prefab& prefab, bool append_resource_path)
{
  // Construct full path
  std::string full_path = append_resource_path ? resource_path + filepath : filepath;

  // Serialize prefab to JSON
  std::string json_content = Engine::FileIO::serialize_prefab_to_json(prefab);

  if (json_content.empty() || json_content == "{}")
  {
    Log::warning("Failed to serialize prefab: " + full_path);
    return false;
  }

  // TODO: this needs to create the folder if it doesn't exist

  // Write to file
  std::ofstream file(full_path);
  if (!file.is_open())
  {
    Log::warning("Failed to open prefab file for writing: " + full_path);
    return false;
  }

  file << json_content;
  file.close();

  Log::info("Prefab saved: " + full_path);

  // Update cache
  prefab_cache[full_path] = prefab;

  return true;
}

void Engine::Resources::clear_unused_resources()
{
  // Clear expired texture cache entries
  for (auto it = texture_cache.begin(); it != texture_cache.end();)
  {
    if (it->second.expired())
    {
      Log::info("Clearing unused texture from cache: " + it->first);
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
      Log::info("Clearing unused font from cache: " + it->first);
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
      Log::info("Clearing unused sound from cache: " + it->first);
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
      Log::info("Clearing unused prefab from cache: " + it->first);
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