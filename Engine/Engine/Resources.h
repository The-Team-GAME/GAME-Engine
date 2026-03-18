//-----------------------------------------------------------------------------------
// file:    Resources.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Header file defining the resource management system interface for the
//          game engine. Provides functions for loading and managing textures,
//          fonts, sounds, and prefabs with automatic caching and memory management.
//          Supports resource statistics tracking and cleanup operations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "TypeDef.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <variant>
#include "Graphics/Renderer.h"
#include "ECS/Prefab.h"
#include "LevelManager.h"
#include "EngineExport.h"

namespace Engine {
  namespace Resources {
    // Variant type to hold any resource type
    using Resource = std::variant<
      std::monostate,  // Empty state for failed loads
      Engine::Graphics::Texture,
      Engine::Graphics::Font,
      Engine::ECS::Prefab,
      Engine::EntityList
    >;
    // Declare as extern - actual definition in Resources.cpp
    ENGINE_API extern std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::TextureData>> texture_cache;
    ENGINE_API extern std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::FontData>> font_cache;
    ENGINE_API extern std::unordered_map<std::string, std::weak_ptr<Engine::ECS::PrefabImpl>> prefab_cache;
    ENGINE_API extern std::unordered_map<std::string, Engine::EntityList> level_cache;

    // Vector to hold shared_ptr references for persistent resources
    ENGINE_API extern std::vector<Resource> persistent_resources;

    struct ResourceStats {
      size_t texture_count;
      size_t texture_alive;
      size_t font_count;
      size_t font_alive;
      size_t prefab_count;
      size_t prefab_alive;
    };

    // Base path for resource loading
    ENGINE_API extern std::string resource_path;

    // Initialization code here (e.g., setting up resource maps, loading default resources)
    ENGINE_API bool init(const std::string& initial_resource_path = "");

    // Cleanup code here (e.g., unloading all resources, clearing maps)
    ENGINE_API bool exit();

    // Set the base path for resource loading
    ENGINE_API void set_resource_path(const std::string& path);

    // Load all assets from the resource path and its subfolders
    ENGINE_API void load_all_assets();

    // Load all prefabs from the prefabs subfolder (always loaded regardless of load_all_assets_on_startup)
    ENGINE_API void load_all_prefabs();

    // Copy a file to the resource folder root (or subfolder) if it's not already there
    ENGINE_API std::string copy_to_resources(const std::string& filepath, const std::string& subfolder = "");

    ENGINE_API std::string get_relative_path(const std::string& full_path);

    ENGINE_API Engine::Graphics::Texture load_texture(const std::string& filepath, bool append_resource_path = true, bool persist = false);
    ENGINE_API Engine::Graphics::Font load_font(const std::string& filepath, u32 size, bool append_resource_path = true, bool persist = false);

    ENGINE_API Engine::Graphics::Texture get_rect_texture();

    // Generic load function - automatically detects resource type from file extension
    ENGINE_API Resource load(const std::string& filepath, bool append_resource_path = true, bool persist = false);

    // Prefab loading and saving
    ENGINE_API Engine::ECS::Prefab load_prefab(const std::string& filepath, bool append_resource_path = true, bool persist = false);
    // Overload: load prefab from pre-parsed GenericValue (avoids re-reading + re-parsing the file)
    ENGINE_API Engine::ECS::Prefab load_prefab_from_generic_value(const std::string& filepath, const ReflectionLibrary::GenericValue& parsed_value, bool append_resource_path = true, bool persist = false);
    ENGINE_API bool save_prefab(const std::string& filepath, const Engine::ECS::Prefab& prefab, bool append_resource_path = true);

    // Add prefab to cache without saving to disk
    ENGINE_API void cache_prefab(const std::string& filepath, const Engine::ECS::Prefab& prefab, bool append_resource_path = true, bool persist = false);

    // Level loading and saving
    ENGINE_API Engine::EntityList load_level(const std::string& filepath, bool append_resource_path = true, bool persist = false);
    // Overload: load level from pre-parsed GenericValue (avoids re-reading + re-parsing the file)
    ENGINE_API Engine::EntityList load_level_from_generic_value(const std::string& filepath, const ReflectionLibrary::GenericValue& parsed_value, bool append_resource_path = true, bool persist = false);
    ENGINE_API bool save_level(const std::string& filepath, const Engine::EntityList& entities, bool append_resource_path = true);

    // Add level to cache without saving to disk
    ENGINE_API void cache_level(const std::string& filepath, const Engine::EntityList& entities, bool append_resource_path = true, bool persist = false);

    // Open (instantiate) a level into the world
    ENGINE_API bool open_level(const std::string& filepath, bool append_resource_path = true);

    // Generic open function - automatically detects resource type and opens it
    // Currently supports: levels
    ENGINE_API bool open(const std::string& filepath, bool append_resource_path = true);

    // Remove a specific resource from all caches (e.g., when file is deleted)
    ENGINE_API void remove_from_cache(const std::string& filepath, bool append_resource_path = true);

    //optimization: clear unused resources from cache
    ENGINE_API void clear_unused_resources();




    ENGINE_API ResourceStats get_resource_stats();

    // Post-processing preset save/load
    ENGINE_API Engine::Graphics::PostProcessing::Settings load_postprocessing_settings(const std::string& filepath, bool append_resource_path = true);
    ENGINE_API bool save_postprocessing_settings(const std::string& filepath, const Engine::Graphics::PostProcessing::Settings& settings, bool append_resource_path = true);
  }
}