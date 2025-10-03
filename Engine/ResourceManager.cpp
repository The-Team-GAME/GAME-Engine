//-----------------------------------------------------------------------------------
// file:    ResourceManager.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file implements the ResourceManager class which provides centralized
//          management of game resources including textures, sounds, fonts, sprite
//          sheets, and object prefabs with automatic duplicate detection.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ResourceManager.h"
#include "Core.h"
#include "ObjectFactory.h"
#include <iostream>
#include <algorithm>
#include <cctype>

/* ===================== //
// Texture Management    //
// ===================== */
s32 Engine::ResourceManager::load_texture(std::string name, std::string path, Graphics::MinifyingFilter minifying_filter)
{
  // Check if texture with this name already exists
  if (textures.find(name) != textures.end())
  {
    std::cout << "ResourceManager: Texture '" << name << "' already loaded (ID: " << textures[name] << "), skipping duplicate load.\n";
    return textures[name];
  }

  // Load the texture
  s32 texture_id = Engine::GRAPHICS.load_texture(path.c_str(), minifying_filter);
  if (texture_id != -1)
  {
    textures[name] = texture_id;
    std::cout << "ResourceManager: Successfully loaded texture '" << name << "' from '" << path << "' (ID: " << texture_id << ").\n";
    return texture_id;
  }
  else
  {
    std::cout << "ResourceManager: Failed to load texture '" << name << "' from '" << path << "'.\n";
    return -1;
  }
}

s32 Engine::ResourceManager::get_texture(std::string name)
{
  auto it = textures.find(name);
  if (it != textures.end())
  {
    return it->second;
  }
  else
  {
    std::cout << "ResourceManager: Texture '" << name << "' not found.\n";
    return -1;
  }
}

void Engine::ResourceManager::unload_texture(std::string name)
{
  auto it = textures.find(name);
  if (it != textures.end())
  {
    std::cout << "ResourceManager: Attempting to unload texture '" << name << "' (ID: " << it->second << ").\n";
    Engine::GRAPHICS.unload_texture(it->second);
    textures.erase(it);
    std::cout << "ResourceManager: Unloaded texture '" << name << "'.\n";
  }
  else
  {
    std::cout << "ResourceManager: Cannot unload texture '" << name << "' - not found in registry.\n";
  }
}

size_t Engine::ResourceManager::get_texture_count()
{
  return textures.size();
}

/* ===================== //
// Sound Management      //
// ===================== */
s32 Engine::ResourceManager::load_sound(std::string name, std::string path)
{
  // Check if sound with this name already exists
  if (sounds.find(name) != sounds.end())
  {
    std::cout << "ResourceManager: Sound '" << name << "' already loaded (ID: " << sounds[name] << "), skipping duplicate load.\n";
    return sounds[name];
  }

  // Load the sound
  s32 soundId = Engine::AUDIO.load_sound(path);
  if (soundId != -1)
  {
    sounds[name] = soundId;
    std::cout << "ResourceManager: Successfully loaded sound '" << name << "' from '" << path << "' (ID: " << soundId << ").\n";
    return soundId;
  }
  else
  {
    std::cout << "ResourceManager: Failed to load sound '" << name << "' from '" << path << "'.\n";
    return -1;
  }
}

s32 Engine::ResourceManager::get_sound(std::string name)
{
  auto it = sounds.find(name);
  if (it != sounds.end())
  {
    return it->second;
  }
  else
  {
    std::cout << "ResourceManager: Sound '" << name << "' not found.\n";
    return -1;
  }
}

void Engine::ResourceManager::unload_sound(std::string name)
{
  auto it = sounds.find(name);
  if (it != sounds.end())
  {
    std::cout << "ResourceManager: Attempting to unload sound '" << name << "' (ID: " << it->second << ").\n";
    Engine::AUDIO.unload_sound(it->second);
    sounds.erase(it);
    std::cout << "ResourceManager: Unloaded sound '" << name << "'.\n";
  }
  else
  {
    std::cout << "ResourceManager: Cannot unload sound '" << name << "' - not found in registry.\n";
  }
}

size_t Engine::ResourceManager::get_sound_count()
{
  return sounds.size();
}

/* ===================== //
// Font Management       //
// ===================== */
s32 Engine::ResourceManager::load_font(std::string name, std::string path, u32 size)
{
  // Check if font with this name already exists
  if (fonts.find(name) != fonts.end())
  {
    std::cout << "ResourceManager: Font '" << name << "' already loaded (ID: " << fonts[name] << "), skipping duplicate load.\n";
    return fonts[name];
  }

  // Load the font
  s32 fontId = Engine::GRAPHICS.load_font(path.c_str(), size);
  if (fontId != -1)
  {
    fonts[name] = fontId;
    std::cout << "ResourceManager: Successfully loaded font '" << name << "' from '" << path << "' (ID: " << fontId << ").\n";
    return fontId;
  }
  else
  {
    std::cout << "ResourceManager: Failed to load font '" << name << "' from '" << path << "'.\n";
    return -1;
  }
}

s32 Engine::ResourceManager::get_font(std::string name)
{
  auto it = fonts.find(name);
  if (it != fonts.end())
  {
    return it->second;
  }
  else
  {
    std::cout << "ResourceManager: Font '" << name << "' not found.\n";
    return -1;
  }
}

void Engine::ResourceManager::unload_font(std::string name)
{
  auto it = fonts.find(name);
  if (it != fonts.end())
  {
    std::cout << "ResourceManager: Attempting to unload font '" << name << "' (ID: " << it->second << ").\n";
    Engine::GRAPHICS.unload_font(it->second);
    fonts.erase(it);
    std::cout << "ResourceManager: Unloaded font '" << name << "'.\n";
  }
  else
  {
    std::cout << "ResourceManager: Cannot unload font '" << name << "' - not found in registry.\n";
  }
}

size_t Engine::ResourceManager::get_font_count()
{
  return fonts.size();
}

/* ===================== //
// Sprite Sheet Management//
// ===================== */
s32 Engine::ResourceManager::load_sprite_sheet(std::string name, std::string path, s32 frame_width, s32 frame_height, s32 total_frames, f32 animation_speed, Graphics::MinifyingFilter minifying_filter)
{
  // Check if sprite sheet with this name already exists
  if (sprite_sheets.find(name) != sprite_sheets.end())
  {
    std::cout << "ResourceManager: Sprite sheet '" << name << "' already loaded (ID: " << sprite_sheets[name] << "), skipping duplicate load.\n";
    return sprite_sheets[name];
  }

  // Load the sprite sheet
  s32 spriteSheetId = Engine::GRAPHICS.load_sprite_sheet(path.c_str(), frame_width, frame_height, total_frames, animation_speed, minifying_filter);
  if (spriteSheetId != -1)
  {
    sprite_sheets[name] = spriteSheetId;
    std::cout << "ResourceManager: Successfully loaded sprite sheet '" << name << "' from '" << path << "' (ID: " << spriteSheetId << ").\n";
    return spriteSheetId;
  }
  else
  {
    std::cout << "ResourceManager: Failed to load sprite sheet '" << name << "' from '" << path << "'.\n";
    return -1;
  }
}

s32 Engine::ResourceManager::get_sprite_sheet(std::string name)
{
  auto it = sprite_sheets.find(name);
  if (it != sprite_sheets.end())
  {
    return it->second;
  }
  else
  {
    std::cout << "ResourceManager: Sprite sheet '" << name << "' not found.\n";
    return -1;
  }
}

void Engine::ResourceManager::unload_sprite_sheet(std::string name)
{
  auto it = sprite_sheets.find(name);
  if (it != sprite_sheets.end())
  {
    std::cout << "ResourceManager: Attempting to unload sprite sheet '" << name << "' (ID: " << it->second << ").\n";
    Engine::GRAPHICS.unload_sprite_sheet(it->second);
    sprite_sheets.erase(it);
    std::cout << "ResourceManager: Unloaded sprite sheet '" << name << "'.\n";
  }
  else
  {
    std::cout << "ResourceManager: Cannot unload sprite sheet '" << name << "' - not found in registry.\n";
  }
}

size_t Engine::ResourceManager::get_sprite_sheet_count()
{
  return sprite_sheets.size();
}

/* ===================== //
// Object Prefab Management//
// ===================== */
s32 Engine::ResourceManager::load_object_prefab(std::string name, std::string path)
{
  // Check if object prefab with this name already exists
  if (object_prefabs.find(name) != object_prefabs.end())
  {
    std::cout << "ResourceManager: Object prefab '" << name << "' already loaded (ID: " << object_prefabs[name] << "), skipping duplicate load.\n";
    return object_prefabs[name];
  }

  // Load the object prefab through the factory
  s32 prefabId = Engine::FACTORY.registerObject(path);
  if (prefabId != -1)
  {
    object_prefabs[name] = prefabId;
    std::cout << "ResourceManager: Successfully loaded object prefab '" << name << "' from '" << path << "' (ID: " << prefabId << ").\n";
    return prefabId;
  }
  else
  {
    std::cout << "ResourceManager: Failed to load object prefab '" << name << "' from '" << path << "'.\n";
    return -1;
  }
}

s32 Engine::ResourceManager::get_object_prefab(std::string name)
{
  auto it = object_prefabs.find(name);
  if (it != object_prefabs.end())
  {
    return it->second;
  }
  else
  {
    std::cout << "ResourceManager: Object prefab '" << name << "' not found.\n";
    return -1;
  }
}

void Engine::ResourceManager::unload_object_prefab(std::string name)
{
  auto it = object_prefabs.find(name);
  if (it != object_prefabs.end())
  {
    std::cout << "ResourceManager: Attempting to unload object prefab '" << name << "' (ID: " << it->second << ").\n";
    Engine::FACTORY.unregisterObject(it->second);
    object_prefabs.erase(it);
    std::cout << "ResourceManager: Unloaded object prefab '" << name << "'.\n";
  }
  else
  {
    std::cout << "ResourceManager: Cannot unload object prefab '" << name << "' - not found in registry.\n";
  }
}

size_t Engine::ResourceManager::get_object_prefab_count()
{
  return object_prefabs.size();
}

/* ===================== //
// Helper Functions      //
// ===================== */
std::string Engine::ResourceManager::get_file_extension(const std::string& path)
{
  size_t dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos)
  {
    return "";
  }

  std::string extension = path.substr(dotPos + 1);

  // Convert to lowercase for case-insensitive comparison
  std::transform(extension.begin(), extension.end(), extension.begin(),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  return extension;
}

s32 Engine::ResourceManager::load(std::string name, std::string path)
{
  std::string extension = get_file_extension(path);

  // Image formats
  if (extension == "png" || extension == "jpg" || extension == "jpeg" ||
    extension == "bmp" || extension == "tga")
  {
    std::cout << "ResourceManager: Loading texture '" << name << "' from '" << path << "'\n";
    return load_texture(name, path, Graphics::MinifyingFilter::Linear);
  }

  // Audio formats
  else if (extension == "wav" || extension == "mp3" || extension == "ogg")
  {
    std::cout << "ResourceManager: Loading sound '" << name << "' from '" << path << "'\n";
    return load_sound(name, path);
  }

  // Font formats
  else if (extension == "ttf" || extension == "otf")
  {
    std::cout << "ResourceManager: Loading font '" << name << "' from '" << path << "' with default size 48\n";
    return load_font(name, path, 48); // Default font size
  }

  // Object prefab format
  else if (extension == "json")
  {
    std::cout << "ResourceManager: Loading object prefab '" << name << "' from '" << path << "'\n";
    return load_object_prefab(name, path);
  }

  // Unknown format
  else
  {
    std::cout << "ResourceManager: Unknown file extension '" << extension << "' for file '" << path << "'\n";
    return -1;
  }
}