#pragma once
#include "ECS/Types.h"

struct DamageEvent {
  ECS::EntityID target;
  int amount;
};

struct WindowResizeEvent {
  int width;
  int height;
};