#pragma once
#include <glm/fwd.hpp>

namespace Graphics
{
  void init();

  void drawRect(float x, float y, float width, float height, glm::vec3 color);

  void exit();
}