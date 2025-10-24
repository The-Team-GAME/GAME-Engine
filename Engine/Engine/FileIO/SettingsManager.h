#pragma once

#include <string>
#include <optional>
#include "../Math/Vector2D.h"
#include "../Reflection/ReflectionLibrary/Macros.h"

namespace Engine {
  namespace FileIO {
    template<typename T>
    class SettingsManager {
    public:
      T settings{};
      bool load(const std::string& file_path);
      bool save(const std::string& file_path);
    };
  }
}


#include "SettingsManager.inl"