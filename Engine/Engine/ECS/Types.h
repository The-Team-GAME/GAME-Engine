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

#include <bitset>
#include <vector>
#include <initializer_list>
#include "../TypeDef.h"

namespace Engine {
  namespace ECS {

    /* ============== //
    // Type aliases   //
    // ============== */
    using EntityID = u32;
    using ComponentTypeID = u32;
    using LayerID = u32;

    /* =================== //
    // Constants           //
    // =================== */
    inline constexpr EntityID INVALID_ENTITY = 0;
    inline constexpr ComponentTypeID INVALID_COMPONENT_TYPE = 0;
    inline constexpr LayerID INVALID_LAYER = 0;
    inline constexpr size_t MAX_COMPONENTS = 64;
    inline constexpr size_t MAX_LAYERS = 32;

    /* ======================== //
    // Bitset signatures        //
    // ======================== */
    using ComponentSignature = std::bitset<MAX_COMPONENTS>;

    /* ======================== //
    // LayerSignature class     //
    // ======================== */
    class LayerSignature : public std::bitset<MAX_LAYERS> {
    public:
      // Default constructor
      LayerSignature() = default;

      // Constructor that takes initializer list of LayerIDs
      LayerSignature(std::initializer_list<LayerID> layers) {
        for (LayerID layer : layers) {
          if (layer < MAX_LAYERS) {
            set(layer);
          }
        }
      }

      // Constructor that takes vector of LayerIDs
      LayerSignature(const std::vector<LayerID>& layers) {
        for (LayerID layer : layers) {
          if (layer < MAX_LAYERS) {
            set(layer);
          }
        }
      }
    };

  } // namespace ECS
} // namespace Engine