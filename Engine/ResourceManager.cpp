#include "ResourceManager.h"
#include "Core.h"

void Engine::ResourceManager::load_texture(std::string name, std::string path)
{
  textures[name] = Engine::GRAPHICS.loadTexture(path.c_str());
}

int Engine::ResourceManager::get_texture(std::string name)
{
  if (textures.find(name) != textures.end())
    return textures[name];
  else
    return -1;
}

void Engine::ResourceManager::unload_texture(std::string name)
{
  Engine::GRAPHICS.unloadTexture(textures[name]);
  textures.erase(name);
}

size_t Engine::ResourceManager::get_texture_count()
{
  return Engine::GRAPHICS.getTextureCount();
}

void Engine::ResourceManager::load_sound(std::string name, std::string path)
{
  sounds[name] = Engine::AUDIO.loadSound(path.c_str());
}

int Engine::ResourceManager::get_sound(std::string name)
{
  if (sounds.find(name) != sounds.end())
    return sounds[name];
  else
    return -1;
}

void Engine::ResourceManager::unload_sound(std::string name)
{
  Engine::AUDIO.unloadSound(sounds[name]);
  sounds.erase(name);
}

size_t Engine::ResourceManager::get_sound_count()
{
  return Engine::AUDIO.getSoundCount(); // Assuming similar method exists for audio
}

void Engine::ResourceManager::load_font(std::string name, std::string path, unsigned int size)
{
  fonts[name] = Engine::GRAPHICS.loadFont(path.c_str(), size);
}

int Engine::ResourceManager::get_font(std::string name)
{
  if (fonts.find(name) != fonts.end())
    return fonts[name];
  else
    return -1;
}

void Engine::ResourceManager::unload_font(std::string name)
{
  Engine::GRAPHICS.unloadFont(fonts[name]);
  fonts.erase(name);
}

size_t Engine::ResourceManager::get_font_count()
{
  return Engine::GRAPHICS.getFontCount();
}

void Engine::ResourceManager::load_sprite_sheet(std::string name, std::string path, int frameWidth, int frameHeight, int totalFrames, float animationSpeed)
{
  spriteSheets[name] = Engine::GRAPHICS.loadSpriteSheet(path.c_str(), frameWidth, frameHeight, totalFrames, animationSpeed);
}

int Engine::ResourceManager::get_sprite_sheet(std::string name)
{
  if (spriteSheets.find(name) != spriteSheets.end())
    return spriteSheets[name];
  else
    return -1;
}

void Engine::ResourceManager::unload_sprite_sheet(std::string name)
{
  Engine::GRAPHICS.unloadSpriteSheet(spriteSheets[name]);
  spriteSheets.erase(name);
}

size_t Engine::ResourceManager::get_sprite_sheet_count()
{
  return Engine::GRAPHICS.getSpriteSheetCount();
}