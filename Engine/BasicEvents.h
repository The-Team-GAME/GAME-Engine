#pragma once
#include "ECS/Types.h"
#include <string>
#include <vector>

struct DamageEvent {
  ECS::EntityID target;
  s32 amount;
};

struct WindowResizeEvent {
  s32 width;
  s32 height;
};

struct FileDropEvent {
  std::vector<std::string> filePaths;
};