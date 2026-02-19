//-----------------------------------------------------------------------------------
// file:    Random.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares the Random namespace which provides functions for
//          generating random values including UUIDs (Universally Unique Identifiers).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "TypeDef.h"
#include "EngineExport.h"

namespace Engine {
  namespace Random {

    /*=================================================================================
    Function Brief:
    Initializes the random number generator with a seed based on current time.
    Should be called once at program startup.

    Parameters:
    None.

    Where to call it:
    During engine initialization in Core.cpp.
    ==================================================================================*/
    ENGINE_API void initialize();

    /*=================================================================================
    Function Brief:
    Generates a Version 4 UUID (random UUID) as a string.
    Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    where x is any hexadecimal digit and y is one of 8, 9, A, or B.

    Parameters:
    None.

    Returns:
    A string containing a randomly generated UUID.

    Where to call it:
    When creating unique identifiers for prefabs, entities, or other resources.
    ==================================================================================*/
    ENGINE_API std::string generate_uuid();

    /*=================================================================================
    Function Brief:
    Generates a random unsigned 32-bit integer.

    Parameters:
    None.

    Returns:
    A random u32 value.

    Where to call it:
    When needing random integer values for game logic.
    ==================================================================================*/
    ENGINE_API u32 random_u32();

    /*=================================================================================
    Function Brief:
    Generates a random unsigned 64-bit integer.

    Parameters:
    None.

    Returns:
    A random u64 value.

    Where to call it:
    When needing large random integer values or unique IDs.
    ==================================================================================*/
    ENGINE_API u64 random_u64();

    /*=================================================================================
    Function Brief:
    Generates a random float between 0.0 and 1.0.

    Parameters:
    None.

    Returns:
    A random float value in the range [0.0, 1.0).

    Where to call it:
    When needing random floating-point values for game logic.
    ==================================================================================*/
    ENGINE_API float random_float();

    /*=================================================================================
    Function Brief:
    Generates a random float in a specified range.

    Parameters:
    min - Minimum value (inclusive).
    max - Maximum value (exclusive).

    Returns:
    A random float value in the range [min, max).

    Where to call it:
    When needing random floating-point values within a specific range.
    ==================================================================================*/
    ENGINE_API float random_range(float min, float max);

  } // namespace Random
} // namespace Engine
