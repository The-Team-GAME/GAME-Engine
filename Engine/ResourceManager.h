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
    std::unordered_map<std::string, int> fonts;
    std::unordered_map<std::string, int> spriteSheets;

  public:
    // TODO: dont pass string as value

    void load_texture(std::string name, std::string path);
    int get_texture(std::string name);
    void unload_texture(std::string name);
    size_t get_texture_count();

    void load_sound(std::string name, std::string path);
    int get_sound(std::string name);
    void unload_sound(std::string name);
    size_t get_sound_count();

    void load_font(std::string name, std::string path, unsigned int size);
    int get_font(std::string name);
    void unload_font(std::string name);
    size_t get_font_count();

    void load_sprite_sheet(std::string name, std::string path, int frameWidth, int frameHeight, int totalFrames, float animationSpeed);
    int get_sprite_sheet(std::string name);
    void unload_sprite_sheet(std::string name);
    size_t get_sprite_sheet_count();
  };
}