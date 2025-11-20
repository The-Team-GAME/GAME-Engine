//-----------------------------------------------------------------------------------
// file:    UtilityComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for utility and metadata purposes.
//          Includes Tag component for compile-time string tagging of entities,
//          and Internal component for marking entities that should not be
//          modified by game developers. Provides organizational utilities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../TypeDef.h"
#include "../ECS/Macros.h"

namespace Components
{
  /* ======================================= //
  // Fixed string for compile-time tags     //
  // ======================================= */
  template<size_t N>
  struct fixed_string {
    constexpr fixed_string(const char(&str)[N]) {
      std::copy_n(str, N, value);
    }
    char value[N];
  };

  template <fixed_string id>
  struct Tag {};

  // Entities with this should not be modifyable by game dev
  struct Internal {};

  // Component for entities that should be destroyed after a certain time
  struct Lifetime {
    f32 duration = 1.0f;       // How long the entity lives (in seconds)
    f32 elapsed = 0.0f;        // Time elapsed since creation

    Lifetime() : duration(1.0f), elapsed(0.0f) {}
    Lifetime(f32 time) : duration(time), elapsed(0.0f) {}
  };
}

REGISTER_COMPONENT(Components::Lifetime, duration, elapsed)