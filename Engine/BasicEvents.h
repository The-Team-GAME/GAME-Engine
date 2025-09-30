#pragma once
#include "ECS/Types.h"
#include <string>
#include <vector>

struct DamageEvent {
  ECS::EntityID target;
  int amount;
};

struct WindowResizeEvent {
  int width;
  int height;
};

struct FileDropEvent {
  std::vector<std::string> filePaths;
};