#pragma once

#include "../ECS/ECS.h"
#include <iostream>

// Assuming Math::Vec2D structure exists, similar to how Math::Vec4D is used in your code.
// If it doesn't, you would need to define it or include the correct header.
namespace Math {
    struct Vec2D {
        float x;
        float y;

        Vec2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
    };
}

namespace Engine
{
    namespace Components
    {
        // Component to make an entity react visually to mouse hover.
        class Hoverable : public Engine::ECS::Component {
        public:
            Math::Vec2D default_scale;   // The original scale of the entity.
            Math::Vec2D hover_scale;     // The scale to apply when mouse is hovering.
            bool is_hovered = false;     // Internal state flag.

            // Constructor
            Hoverable(const Math::Vec2D& default_scale, const Math::Vec2D& hover_scale)
                : default_scale(default_scale), hover_scale(hover_scale) {}

            // Helper constructor taking raw float values for simplicity
            Hoverable(float default_x, float default_y, float hover_x, float hover_y)
                : default_scale(default_x, default_y), hover_scale(hover_x, hover_y) {}
        };
    } // namespace Components
} // namespace Engine