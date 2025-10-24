#pragma once

#include <string>
#include "../Math/Vector2D.h"
#include "../Reflection/ReflectionLibrary/Macros.h"



namespace Engine {
  namespace FileIO {
    struct Settings {
      Math::Vec2D window_size = { 1600.0f, 900.0f };
    };

    bool load_settings(const std::string& file_path);
    bool save_settings(const Settings& settings, const std::string& file_path);

  }
}


REFLECT_FIELDS(Engine::FileIO::Settings, window_size)