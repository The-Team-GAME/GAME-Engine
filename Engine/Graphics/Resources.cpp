#include "Resources.h"
#include <memory>
#include "RendererInternal.h"
#include "../Logger.h"

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

  // unit_rect will be automatically cleaned up when it goes out of scope
  unit_rect.reset();

  Log::info("Resource manager cleaned up");
  return true;
}

Engine::Graphics::Texture Engine::Resources::load_texture(const std::string& filepath)
{
  // Check if texture is already in cache
  auto it = texture_cache.find(filepath);
  if (it != texture_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      Log::info("Texture loaded from cache: " + filepath);
      return cached;
    }
    else
    {
      // Expired, remove from cache
      texture_cache.erase(it);
    }
  }

  // Load new texture
  auto texture = std::make_shared<Engine::Graphics::TextureData>(filepath);

  // Only cache if the texture was successfully loaded
  if (texture->id != 0)
  {
    texture_cache[filepath] = texture;
    Log::info("Texture loaded: " + filepath);
  }
  else
  {
    Log::warning("Failed to load texture: " + filepath);
  }

  return texture;
}

Engine::Graphics::Font Engine::Resources::load_font(const std::string& filepath, u32 size)
{
  // Create a unique key combining filepath and size
  std::string cache_key = filepath + "_" + std::to_string(size);

  // Check if font is already in cache
  auto it = font_cache.find(cache_key);
  if (it != font_cache.end())
  {
    // Try to lock the weak_ptr
    if (auto cached = it->second.lock())
    {
      Log::info("Font loaded from cache: " + filepath + " (size: " + std::to_string(size) + ")");
      return cached;
    }
    else
    {
      // Expired, remove from cache
      font_cache.erase(it);
    }
  }

  // Load new font
  auto font = std::make_shared<Engine::Graphics::FontData>(filepath, size);

  // Cache the font
  font_cache[cache_key] = font;
  Log::info("Font loaded: " + filepath + " (size: " + std::to_string(size) + ")");

  return font;
}

Engine::Graphics::Texture Engine::Resources::get_unit_rect_texture()
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
}
