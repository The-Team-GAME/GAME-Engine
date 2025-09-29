#pragma once
#include <unordered_map>
#include <string>

namespace Engine
{
  class ResourceManager
  {
  private:
    std::unordered_map<std::string, int> textures;
    std::unordered_map<std::string, int> sounds;

  public:
    // TODO: dont pass string as value

    void load_texture(std::string name, std::string path);
    int get_texture(std::string name);
    void unload_texture(std::string name);

    void load_sound(std::string name, std::string path);
    int get_sound(std::string name);
    void unload_sound(std::string name);
  };
}