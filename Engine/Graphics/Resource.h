#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "Types.h"
#include "../TypeDef.h"

namespace Engine {
  namespace Resources {
    static std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::TextureData>> texture_cache;
    static std::unordered_map<std::string, std::weak_ptr<Engine::Graphics::FontData>> font_cache;

    // TODO: unit_rect does not need reference counting, since it's always in memory until we exit
    static Engine::Graphics::Texture unit_rect;

    // Initialization code here (e.g., setting up resource maps, loading default resources)
    bool init();

    // Cleanup code here (e.g., unloading all resources, clearing maps)
    bool exit();

    Engine::Graphics::Texture load_texture(const std::string& filepath);
    Engine::Graphics::Font load_font(const std::string& filepath, u32 size);

    Engine::Graphics::Texture get_unit_rect_texture();

    //optimization: clear unused resources from cache
    void clear_unused_resources();
  }
}