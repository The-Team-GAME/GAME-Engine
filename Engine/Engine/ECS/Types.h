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
#include <string>
#include "../TypeDef.h"

namespace Engine {
  namespace ECS {

    /* ============== //
    // Type aliases   //
    // ============== */
    using EntityID = size_t;
    using ComponentTypeID = size_t;
    using LayerID = size_t;
    using EntityPairHash = size_t; // Used for hashing entity pairs in for_each_entity_pair

    /* =================== //
    // Constants           //
    // =================== */
    inline constexpr EntityID INVALID_ENTITY = 0;
    inline constexpr ComponentTypeID INVALID_COMPONENT_TYPE = 0;
    inline constexpr LayerID INVALID_LAYER = 0;
    inline constexpr LayerID DEFAULT_LAYER = 1;
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
      LayerSignature(std::initializer_list<LayerID> layers);

      // Constructor that takes vector of LayerIDs
      LayerSignature(const std::vector<LayerID>& layers);

      // Constructor that takes initializer list of layer names (strings)
      LayerSignature(std::initializer_list<const char*> layer_names);

      // Constructor that takes vector of layer names (strings)
      LayerSignature(const std::vector<std::string>& layer_names);
    };

  } // namespace ECS
} // namespace Engine