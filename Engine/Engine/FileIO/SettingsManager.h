#pragma once

#include <string>
#include <optional>
#include "../Math/Vector2D.h"
#include "../Reflection/ReflectionLibrary/Macros.h"

namespace Engine {
  namespace FileIO {
    template<typename T>
    class SettingsManager {
    private:
      T settings{};

    public:
      bool load(const std::string& file_path);
      bool save(const std::string& file_path);
    };
  }
}
