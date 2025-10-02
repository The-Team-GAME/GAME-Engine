//-----------------------------------------------------------------------------------
// file:    ResourceManager.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file defines the ResourceManager class which provides centralized
//          management of game resources including textures, sounds, fonts, sprite
//          sheets, and object prefabs with automatic duplicate detection.
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
  class ResourceManager
  {
  private:
    std::unordered_map<std::string, int> textures;
    std::unordered_map<std::string, int> sounds;
    std::unordered_map<std::string, int> fonts;
    std::unordered_map<std::string, int> spriteSheets;
    std::unordered_map<std::string, int> objectPrefabs;

  public:
    /* ===================== //
    // Texture Management    //
    // ===================== */
    /*=================================================================================
    Function Brief:
    Loads a texture from file and registers it with a unique name. Prevents duplicate
    loading by checking if the name already exists.

    Parameters:
    name - Unique identifier for the texture
    path - File path to the texture image
    minifyingFilter - Filtering mode for texture minification

    Returns:
    The texture ID if successful, or existing ID if already loaded, -1 on failure.
    ==================================================================================*/
    s32 load_texture(std::string name, std::string path, Graphics::MinifyingFilter minifyingFilter);
    s32 get_texture(std::string name);
    void unload_texture(std::string name);
    size_t get_texture_count();
    const std::unordered_map<std::string, int>& get_all_textures() const { return textures; }

    /* ===================== //
    // Sound Management      //
    // ===================== */
    /*=================================================================================
    Function Brief:
    Loads a sound from file and registers it with a unique name. Prevents duplicate
    loading by checking if the name already exists.

    Parameters:
    name - Unique identifier for the sound
    path - File path to the sound file

    Returns:
    The sound ID if successful, or existing ID if already loaded, -1 on failure.
    ==================================================================================*/
    s32 load_sound(std::string name, std::string path);
    s32 get_sound(std::string name);
    void unload_sound(std::string name);
    size_t get_sound_count();
    const std::unordered_map<std::string, int>& get_all_sounds() const { return sounds; }

    /* ===================== //
    // Font Management       //
    // ===================== */
    /*=================================================================================
    Function Brief:
    Loads a font from file at a specified size and registers it with a unique name.
    Prevents duplicate loading by checking if the name already exists.

    Parameters:
    name - Unique identifier for the font
    path - File path to the font file
    size - Font size in pixels

    Returns:
    The font ID if successful, or existing ID if already loaded, -1 on failure.
    ==================================================================================*/
    s32 load_font(std::string name, std::string path, u32 size);
    s32 get_font(std::string name);
    void unload_font(std::string name);
    size_t get_font_count();
    const std::unordered_map<std::string, int>& get_all_fonts() const { return fonts; }

    /* ===================== //
    // Sprite Sheet Management//
    // ===================== */
    /*=================================================================================
    Function Brief:
    Loads a sprite sheet from file and registers it with animation parameters.
    Prevents duplicate loading by checking if the name already exists.

    Parameters:
    name - Unique identifier for the sprite sheet
    path - File path to the sprite sheet image
    frameWidth - Width of each frame in pixels
    frameHeight - Height of each frame in pixels
    totalFrames - Total number of frames in the sprite sheet
    animationSpeed - Frames per second for animation playback
    minifyingFilter - Filtering mode for texture minification

    Returns:
    The sprite sheet ID if successful, or existing ID if already loaded, -1 on failure.
    ==================================================================================*/
    s32 load_sprite_sheet(std::string name, std::string path, s32 frameWidth, s32 frameHeight,
      s32 totalFrames, f32 animationSpeed, Graphics::MinifyingFilter minifyingFilter);
    s32 get_sprite_sheet(std::string name);
    void unload_sprite_sheet(std::string name);
    size_t get_sprite_sheet_count();
    const std::unordered_map<std::string, int>& get_all_sprite_sheets() const { return spriteSheets; }

    /* ===================== //
    // Object Prefab Management//
    // ===================== */
    /*=================================================================================
    Function Brief:
    Loads an object prefab from JSON file and registers it with the ObjectFactory.
    Prevents duplicate loading by checking if the name already exists.

    Parameters:
    name - Unique identifier for the object prefab
    path - File path to the JSON configuration file

    Returns:
    The prefab ID if successful, or existing ID if already loaded, -1 on failure.
    ==================================================================================*/
    s32 load_object_prefab(std::string name, std::string path);
    s32 get_object_prefab(std::string name);
    void unload_object_prefab(std::string name);
    size_t get_object_prefab_count();
    const std::unordered_map<std::string, int>& get_all_object_prefabs() const { return objectPrefabs; }

    /* ===================== //
    // Generic Loading       //
    // ===================== */
    /*=================================================================================
    Function Brief:
    Generic resource loading function that automatically detects file type from
    extension and calls the appropriate specialized load function.

    Parameters:
    name - Unique identifier for the resource
    path - File path to the resource

    Returns:
    The resource ID if successful, -1 on failure or unsupported file type.
    ==================================================================================*/
    s32 load(std::string name, std::string path);

  private:
    std::string get_file_extension(const std::string& path);
  };
}