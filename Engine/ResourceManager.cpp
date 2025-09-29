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
