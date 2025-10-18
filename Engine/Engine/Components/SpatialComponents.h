#pragma once

#include "../Math/Vector2D.h"

namespace Components
{
  struct Transform2D
  {
    Math::Vec2D position;
    float rotation = 0.0f; // In degrees
    Math::Vec2D scale = { 1.0f, 1.0f };

    Transform2D() : position(0.0f, 0.0f), rotation(0.0f), scale(1.0f, 1.0f) {}
    Transform2D(const Math::Vec2D& pos, float rot = 0.0f, const Math::Vec2D& scl = Math::Vec2D(1.0f, 1.0f))
      : position(pos), rotation(rot), scale(scl) {
    }
    Transform2D(float x, float y, float rot, float scaleX, float scaleY)
      : position(x, y), rotation(rot), scale(scaleX, scaleY) {
    }
  };

  // NOT USED YET, WILL BE IMPLEMENTED IN THE FUTURE

  struct ZIndex {
    int layer = 0; // For rendering order
  };

  struct ScreenTransform2D
  {
    Math::Vec2D position;
    float rotation = 0.0f; // In degrees
    Math::Vec2D scale = { 1.0f, 1.0f };

    ScreenTransform2D() : position(0.0f, 0.0f), rotation(0.0f), scale(1.0f, 1.0f) {}
    ScreenTransform2D(const Math::Vec2D& pos, float rot = 0.0f, const Math::Vec2D& scl = Math::Vec2D(1.0f, 1.0f))
      : position(pos), rotation(rot), scale(scl) {
    }
    ScreenTransform2D(float x, float y, float rot, float scaleX, float scaleY)
      : position(x, y), rotation(rot), scale(scaleX, scaleY) {
    }
  };
}

REFLECT_FIELDS(Components::Transform2D, position, rotation, scale)
REFLECT_FIELDS(Components::ScreenTransform2D, position, rotation, scale)
