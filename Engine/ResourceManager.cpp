#include "ResourceManager.h"
#include "Core.h"
#include <iostream>

int Engine::ResourceManager::load_texture(std::string name, std::string path, Graphics::MinifyingFilter minifyingFilter)
{
  // Check if texture with this name already exists
  if (textures.find(name) != textures.end())
  {
    std::cout << "ResourceManager: Texture '" << name << "' already loaded (ID: " << textures[name] << "), skipping duplicate load.\n";
    return textures[name];
  }

  // Load the texture
  int textureId = Engine::GRAPHICS.loadTexture(path.c_str(), minifyingFilter);
  if (textureId != -1)
  {
    textures[name] = textureId;
    std::cout << "ResourceManager: Successfully loaded texture '" << name << "' from '" << path << "' (ID: " << textureId << ").\n";
    return textureId;
  }
  else
  {
    std::cout << "ResourceManager: Failed to load texture '" << name << "' from '" << path << "'.\n";
    return -1;
  }
}

int Engine::ResourceManager::get_texture(std::string name)
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
    Engine::GRAPHICS.unloadTexture(it->second);
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

int Engine::ResourceManager::load_sound(std::string name, std::string path)
{
  // Check if sound with this name already exists
  if (sounds.find(name) != sounds.end())
  {
    std::cout << "ResourceManager: Sound '" << name << "' already loaded (ID: " << sounds[name] << "), skipping duplicate load.\n";
    return sounds[name];
  }

  // Load the sound
  int soundId = Engine::AUDIO.loadSound(path);
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

int Engine::ResourceManager::get_sound(std::string name)
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
    Engine::AUDIO.unloadSound(it->second);
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

int Engine::ResourceManager::load_font(std::string name, std::string path, unsigned int size)
{
  // Check if font with this name already exists
  if (fonts.find(name) != fonts.end())
  {
    std::cout << "ResourceManager: Font '" << name << "' already loaded (ID: " << fonts[name] << "), skipping duplicate load.\n";
    return fonts[name];
  }

  // Load the font
  int fontId = Engine::GRAPHICS.loadFont(path.c_str(), size);
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

int Engine::ResourceManager::get_font(std::string name)
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
    Engine::GRAPHICS.unloadFont(it->second);
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

int Engine::ResourceManager::load_sprite_sheet(std::string name, std::string path, int frameWidth, int frameHeight, int totalFrames, float animationSpeed, Graphics::MinifyingFilter minifyingFilter)
{
  // Check if sprite sheet with this name already exists
  if (spriteSheets.find(name) != spriteSheets.end())
  {
    std::cout << "ResourceManager: Sprite sheet '" << name << "' already loaded (ID: " << spriteSheets[name] << "), skipping duplicate load.\n";
    return spriteSheets[name];
  }

  // Load the sprite sheet
  int spriteSheetId = Engine::GRAPHICS.loadSpriteSheet(path.c_str(), frameWidth, frameHeight, totalFrames, animationSpeed, minifyingFilter);
  if (spriteSheetId != -1)
  {
    spriteSheets[name] = spriteSheetId;
    std::cout << "ResourceManager: Successfully loaded sprite sheet '" << name << "' from '" << path << "' (ID: " << spriteSheetId << ").\n";
    return spriteSheetId;
  }
  else
  {
    std::cout << "ResourceManager: Failed to load sprite sheet '" << name << "' from '" << path << "'.\n";
    return -1;
  }
}

int Engine::ResourceManager::get_sprite_sheet(std::string name)
{
  auto it = spriteSheets.find(name);
  if (it != spriteSheets.end())
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
  auto it = spriteSheets.find(name);
  if (it != spriteSheets.end())
  {
    std::cout << "ResourceManager: Attempting to unload sprite sheet '" << name << "' (ID: " << it->second << ").\n";
    Engine::GRAPHICS.unloadSpriteSheet(it->second);
    spriteSheets.erase(it);
    std::cout << "ResourceManager: Unloaded sprite sheet '" << name << "'.\n";
  }
  else
  {
    std::cout << "ResourceManager: Cannot unload sprite sheet '" << name << "' - not found in registry.\n";
  }
}

size_t Engine::ResourceManager::get_sprite_sheet_count()
{
  return spriteSheets.size();
}