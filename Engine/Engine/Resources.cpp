#include "Resources.h"
#include <memory>
#include "Graphics/RendererInternal.h"
#include "Audio/AudioPlayerInternal.h"
#include "Logger.h"

// Define the actual storage for the extern variables
std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::TextureData>> Engine::Resources::texture_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::FontData>> Engine::Resources::font_cache;
std::unordered_map<std::string, std::weak_ptr<Engine::Audio::SoundData>> Engine::Resources::sound_cache;

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

Engine::Graphics::Texture Engine::Resources::load_texture(const std::string& filepath)
{
  // Construct full path
  std::string full_path = resource_path + filepath;

  // Check if texture is already in cache (use full path for cache key)
  auto it = texture_cache.find(full_path);
  if (it != texture_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      Log::info("Texture loaded from cache: " + full_path);
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
    Log::info("Texture loaded: " + full_path);
  }
  else
  {
    Log::warning("Failed to load texture: " + full_path);
  }

  return texture;
}

Engine::Graphics::Font Engine::Resources::load_font(const std::string& filepath, u32 size)
{
  // Construct full path
  std::string full_path = resource_path + filepath;

  // Create a unique key combining filepath and size
  std::string cache_key = full_path + "_" + std::to_string(size);

  // Check if font is already in cache
  auto it = font_cache.find(cache_key);
  if (it != font_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      Log::info("Font loaded from cache: " + full_path + " (size: " + std::to_string(size) + ")");
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
  Log::info("Font loaded: " + full_path + " (size: " + std::to_string(size) + ")");

  return font;
}

Engine::Audio::Sound Engine::Resources::load_sound(const std::string& filepath)
{
  std::string full_path = resource_path + filepath;

  // Check if sound is already in cache
  auto it = sound_cache.find(full_path);
  if (it != sound_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      Log::info("Sound loaded from cache: " + full_path);
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
    Log::info("Sound loaded: " + full_path);
  }
  else
  {
    Log::warning("Failed to load sound: " + full_path);
  }

  return sound;
}

Engine::Audio::Sound Engine::Resources::load_music(const std::string& filepath)
{
  std::string full_path = resource_path + filepath;

  // Check if music is already in cache
  auto it = sound_cache.find(full_path);
  if (it != sound_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      Log::info("Music loaded from cache: " + full_path);
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
    Log::info("Music loaded: " + full_path);
  }
  else
  {
    Log::warning("Failed to load music: " + full_path);
  }

  return music;
}

Engine::Graphics::Texture Engine::Resources::get_rect_texture()
{
  return unit_rect;
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

  return stats;
}