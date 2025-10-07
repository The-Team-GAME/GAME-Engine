//-----------------------------------------------------------------------------------
// file:    Types.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the core type aliases and constants used throughout
//          the ECS (Entity Component System). Includes entity IDs, component type IDs,
//          event type IDs, and bitset signatures for efficient component matching.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <bitset>
#include <iostream> //remove when not in use
#include "../TypeDef.h"

namespace ECS {

  /* ============== //
  // Type aliases   //
  // ============== */
  using EntityID = u32;
  using ComponentTypeID = u32;
  using EventTypeID = u32;

  /* =================== //
  // Constants           //
  // =================== */
  inline constexpr EntityID INVALID_ENTITY = 0;
  inline constexpr ComponentTypeID INVALID_COMPONENT_TYPE = 0;
  inline constexpr EventTypeID INVALID_EVENT_TYPE = 0;
  inline constexpr size_t MAX_COMPONENTS = 64;
  inline constexpr size_t MAX_EVENTS = 64;

  /* ======================== //
  // Bitset signatures        //
  // ======================== */
  using ComponentSignature = std::bitset<MAX_COMPONENTS>;
  using EventSignature = std::bitset<MAX_EVENTS>;

} // namespace ECS