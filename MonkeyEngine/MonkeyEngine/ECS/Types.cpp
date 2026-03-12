//-----------------------------------------------------------------------------------
// file:    Types.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements LayerSignature string-based constructors.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Types.h"
#include "World.h"

namespace Engine {
  namespace ECS
  {
    // ============================================================================
    // LayerSignature Constructor Implementations
    // ============================================================================

    LayerSignature::LayerSignature(std::initializer_list<LayerID> layers) {
      for (LayerID layer : layers) {
        if (layer < MAX_LAYERS) {
          set(layer);
        }
      }
    }

    LayerSignature::LayerSignature(const std::vector<LayerID>& layers) {
      for (LayerID layer : layers) {
        if (layer < MAX_LAYERS) {
          set(layer);
        }
      }
    }

    LayerSignature::LayerSignature(std::initializer_list<const char*> layer_names) {
      for (const char* name : layer_names) {
        LayerID layer = get_layer_id(name);
        if (layer != INVALID_LAYER) {
          set(layer);
        }
      }
    }

    LayerSignature::LayerSignature(const std::vector<std::string>& layer_names) {
      for (const std::string& name : layer_names) {
        LayerID layer = get_layer_id(name);
        if (layer != INVALID_LAYER) {
          set(layer);
        }
      }
    }

  } // namespace ECS
} // namespace Engine
