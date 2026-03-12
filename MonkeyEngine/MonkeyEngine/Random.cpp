//-----------------------------------------------------------------------------------
// file:    Random.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the Random namespace functions for generating
//          random values including UUIDs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Random.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace Engine {
  namespace Random {

    // Static random number generator
    static std::mt19937_64 rng;
    static bool initialized = false;

    void initialize() {
      // Seed with current time
      std::random_device rd;
      rng.seed(rd());
      initialized = true;
    }

    std::string generate_uuid() {
      // Ensure RNG is initialized
      if (!initialized) {
        initialize();
      }

      // Generate random bytes
      std::uniform_int_distribution<u32> dist(0, 255);

      // UUID format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
      // where x is any hex digit and y is one of 8, 9, A, or B

      std::stringstream ss;
      ss << std::hex << std::setfill('0');

      // First group: 8 hex digits (4 bytes)
      for (int i = 0; i < 4; ++i) {
        ss << std::setw(2) << dist(rng);
      }
      ss << '-';

      // Second group: 4 hex digits (2 bytes)
      for (int i = 0; i < 2; ++i) {
        ss << std::setw(2) << dist(rng);
      }
      ss << '-';

      // Third group: 4 hex digits with version (2 bytes)
      // Set version to 4 (random UUID)
      ss << '4'; // Version 4
      ss << std::setw(2) << (dist(rng) & 0x0F); // First byte with version nibble
      for (int i = 0; i < 1; ++i) {
        ss << std::setw(2) << dist(rng);
      }
      ss << '-';

      // Fourth group: 4 hex digits with variant (2 bytes)
      // Set variant to 10xx (RFC 4122)
      u8 variant_byte = (dist(rng) & 0x3F) | 0x80; // Set two most significant bits to 10
      ss << std::setw(2) << static_cast<u32>(variant_byte);
      for (int i = 0; i < 1; ++i) {
        ss << std::setw(2) << dist(rng);
      }
      ss << '-';

      // Fifth group: 12 hex digits (6 bytes)
      for (int i = 0; i < 6; ++i) {
        ss << std::setw(2) << dist(rng);
      }

      return ss.str();
    }

    u32 random_u32() {
      if (!initialized) {
        initialize();
      }
      std::uniform_int_distribution<u32> dist;
      return dist(rng);
    }

    u64 random_u64() {
      if (!initialized) {
        initialize();
      }
      std::uniform_int_distribution<u64> dist;
      return dist(rng);
    }

    float random_float() {
      if (!initialized) {
        initialize();
      }
      std::uniform_real_distribution<float> dist(0.0f, 1.0f);
      return dist(rng);
    }

    float random_range(float min, float max) {
      if (!initialized) {
        initialize();
      }
      std::uniform_real_distribution<float> dist(min, max);
      return dist(rng);
    }

  } // namespace Random
} // namespace Engine
