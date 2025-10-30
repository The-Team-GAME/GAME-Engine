#pragma once
#include "TypeDef.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <variant>
#include "Graphics/Renderer.h"
#include "Audio/AudioPlayer.h"
#include "ECS/Prefab.h"

namespace Engine {
  namespace Resources {
    // Variant type to hold any resource type
    using Resource = std::variant<
      std::monostate,  // Empty state for failed loads
      Engine::Graphics::Texture,
      Engine::Graphics::Font,
      Engine::Audio::Sound,
      Engine::ECS::Prefab
    >;
    // Declare as extern - actual definition in Resources.cpp
    extern std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::TextureData>> texture_cache;
    extern std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::FontData>> font_cache;
    extern std::unordered_map<std::string, std::weak_ptr<Engine::Audio::SoundData>> sound_cache;
    extern std::unordered_map<std::string, std::weak_ptr<Engine::ECS::PrefabImpl>> prefab_cache;

    // Vector to hold shared_ptr references for persistent resources
    extern std::vector<Resource> persistent_resources;

    struct ResourceStats {
      size_t texture_count;
      size_t texture_alive;
      size_t font_count;
      size_t font_alive;
      size_t sound_count;
      size_t sound_alive;
      size_t prefab_count;
      size_t prefab_alive;
    };

    // TODO: unit_rect does not need reference counting, since it's always in memory until we exit
    extern Engine::Graphics::Texture unit_rect;

    // Base path for resource loading
    extern std::string resource_path;

    // Initialization code here (e.g., setting up resource maps, loading default resources)
    bool init();

    // Cleanup code here (e.g., unloading all resources, clearing maps)
    bool exit();

    // Set the base path for resource loading
    void set_resource_path(const std::string& path);

    std::string get_relative_path(const std::string& full_path);

    Engine::Graphics::Texture load_texture(const std::string& filepath, bool append_resource_path = true, bool persist = false);
    Engine::Graphics::Font load_font(const std::string& filepath, u32 size, bool append_resource_path = true, bool persist = false);

    // Load sound effect (stored in memory)
    Engine::Audio::Sound load_sound(const std::string& filepath, bool append_resource_path = true, bool persist = false);

    // Load music (streamed from disk)
    Engine::Audio::Sound load_music(const std::string& filepath, bool append_resource_path = true, bool persist = false);

    Engine::Graphics::Texture get_rect_texture();

    // Generic load function - automatically detects resource type from file extension
    Resource load(const std::string& filepath, bool append_resource_path = true, bool persist = false);

    // Prefab loading and saving
    Engine::ECS::Prefab load_prefab(const std::string& filepath, bool append_resource_path = true, bool persist = false);
    bool save_prefab(const std::string& filepath, const Engine::ECS::Prefab& prefab, bool append_resource_path = true);

    //optimization: clear unused resources from cache
    void clear_unused_resources();

    ResourceStats get_resource_stats();
  }
}