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
#include "../Time.h"
#include <string>

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
    f32 destroy_at = 0.0f;     // Time (from Engine::Time::get_time()) when entity should be destroyed

    Lifetime() : destroy_at(Engine::Time::get_time()) {}
    Lifetime(f32 duration) : destroy_at(Engine::Time::get_time() + duration) {}
  };

  // Component for entities that should be destroyed after a certain number of frames
  struct FrameLifetime {
    s32 frames_remaining = 1;  // How many more frames the entity should live

    FrameLifetime() : frames_remaining(1) {}
    FrameLifetime(s32 frame_count) : frames_remaining(frame_count) {}
  };

  // Component for naming entities - displays in editor and can be used for lookups
  struct Name {
    std::string name;

    Name() : name("Entity") {}
    Name(const std::string& n) : name(n) {}
    Name(const char* n) : name(n) {}
  };

}

REGISTER_COMPONENT(Components::Lifetime, destroy_at)
REGISTER_COMPONENT(Components::FrameLifetime, frames_remaining)
REGISTER_COMPONENT(Components::Name, name)