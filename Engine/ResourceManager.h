#pragma once
#include <unordered_map>
#include <string>
#include "Graphics.h"

// Forward declaration for ObjectPrefab
struct ObjectPrefab;

namespace Engine
{
  class ResourceManager
  {
  private:
    std::unordered_map<std::string, int> textures;
    std::unordered_map<std::string, int> sounds;
    std::unordered_map<std::string, int> fonts;
    std::unordered_map<std::string, int> spriteSheets;
    std::unordered_map<std::string, int> objectPrefabs;

  public:
    // TODO: dont pass string as value

    int load_texture(std::string name, std::string path, Graphics::MinifyingFilter minifyingFilter);
    int get_texture(std::string name);
    void unload_texture(std::string name);
    size_t get_texture_count();

    int load_sound(std::string name, std::string path);
    int get_sound(std::string name);
    void unload_sound(std::string name);
    size_t get_sound_count();

    int load_font(std::string name, std::string path, unsigned int size);
    int get_font(std::string name);
    void unload_font(std::string name);
    size_t get_font_count();

    int load_sprite_sheet(std::string name, std::string path, int frameWidth, int frameHeight, int totalFrames, float animationSpeed, Graphics::MinifyingFilter minifyingFilter);
    int get_sprite_sheet(std::string name);
    void unload_sprite_sheet(std::string name);
    size_t get_sprite_sheet_count();

    // Object prefab management
    int load_object_prefab(std::string name, std::string path);
    int get_object_prefab(std::string name);
    void unload_object_prefab(std::string name);
    size_t get_object_prefab_count();
  };
}