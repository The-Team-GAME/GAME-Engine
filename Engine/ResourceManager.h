//-----------------------------------------------------------------------------------
// file:    ResourceManager.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Defines the ResourceManager class, which serves as the central registry
//          for all engine assets (textures, sounds, fonts, prefabs, etc.). It
//          provides standardized interfaces for loading, retrieving, and unloading
//          resources, delegating the actual loading work to respective subsystems.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include <unordered_map>
#include <string>
#include "Graphics.h"

// Forward declaration for ObjectPrefab
struct ObjectPrefab;

namespace Engine
{
  /*=================================================================================
      Class Brief:
      The ResourceManager class manages the lifecycle and registry of all assets
      used by the engine. It maps user-friendly string names to internal integer IDs
      provided by subsystems (Graphics, Audio, ObjectFactory). It includes type-specific
      management functions and a generic loader based on file extension.
      ==================================================================================*/
  class ResourceManager
  {
  private:
    // Internal maps to store resource names and their corresponding subsystem IDs
    std::unordered_map<std::string, int> textures;
    std::unordered_map<std::string, int> sounds;
    std::unordered_map<std::string, int> fonts;
    std::unordered_map<std::string, int> spriteSheets;
    std::unordered_map<std::string, int> objectPrefabs;

  public:
    // Texture management
    int load_texture(std::string name, std::string path, Graphics::MinifyingFilter minifyingFilter);
    int get_texture(std::string name);
    void unload_texture(std::string name);
    size_t get_texture_count();
    const std::unordered_map<std::string, int>& get_all_textures() const { return textures; }

    // Sound management
    int load_sound(std::string name, std::string path);
    int get_sound(std::string name);
    void unload_sound(std::string name);
    size_t get_sound_count();
    const std::unordered_map<std::string, int>& get_all_sounds() const { return sounds; }

    // Font management
    int load_font(std::string name, std::string path, unsigned int size);
    int get_font(std::string name);
    void unload_font(std::string name);
    size_t get_font_count();
    const std::unordered_map<std::string, int>& get_all_fonts() const { return fonts; }

    // Sprite sheet management
    int load_sprite_sheet(std::string name, std::string path, int frameWidth, int frameHeight,
      int totalFrames, float animationSpeed, Graphics::MinifyingFilter minifyingFilter);
    int get_sprite_sheet(std::string name);
    void unload_sprite_sheet(std::string name);
    size_t get_sprite_sheet_count();
    const std::unordered_map<std::string, int>& get_all_sprite_sheets() const { return spriteSheets; }

    // Object prefab management
    int load_object_prefab(std::string name, std::string path);
    int get_object_prefab(std::string name);
    void unload_object_prefab(std::string name);
    size_t get_object_prefab_count();
    const std::unordered_map<std::string, int>& get_all_object_prefabs() const { return objectPrefabs; }

    // Generic load function (uses file extension to dispatch to the correct loader)
    int load(std::string name, std::string path);

  private:
    // Utility function to extract and normalize the file extension
    std::string get_file_extension(const std::string& path);
  };
}