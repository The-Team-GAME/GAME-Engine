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
    // Texture management
    s32 load_texture(std::string name, std::string path, Graphics::MinifyingFilter minifyingFilter);
    s32 get_texture(std::string name);
    void unload_texture(std::string name);
    size_t get_texture_count();
    const std::unordered_map<std::string, int>& get_all_textures() const { return textures; }

    // Sound management
    s32 load_sound(std::string name, std::string path);
    s32 get_sound(std::string name);
    void unload_sound(std::string name);
    size_t get_sound_count();
    const std::unordered_map<std::string, int>& get_all_sounds() const { return sounds; }

    // Font management
    s32 load_font(std::string name, std::string path, u32 size);
    s32 get_font(std::string name);
    void unload_font(std::string name);
    size_t get_font_count();
    const std::unordered_map<std::string, int>& get_all_fonts() const { return fonts; }

    // Sprite sheet management
    s32 load_sprite_sheet(std::string name, std::string path, s32 frameWidth, s32 frameHeight,
      s32 totalFrames, f32 animationSpeed, Graphics::MinifyingFilter minifyingFilter);
    s32 get_sprite_sheet(std::string name);
    void unload_sprite_sheet(std::string name);
    size_t get_sprite_sheet_count();
    const std::unordered_map<std::string, int>& get_all_sprite_sheets() const { return spriteSheets; }

    // Object prefab management
    s32 load_object_prefab(std::string name, std::string path);
    s32 get_object_prefab(std::string name);
    void unload_object_prefab(std::string name);
    size_t get_object_prefab_count();
    const std::unordered_map<std::string, int>& get_all_object_prefabs() const { return objectPrefabs; }

    // Generic load function
    s32 load(std::string name, std::string path);

  private:
    std::string get_file_extension(const std::string& path);
  };
}