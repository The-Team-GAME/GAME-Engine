//-----------------------------------------------------------------------------------
// file:    ComponentManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ComponentTypeManager which automatically generates
//          unique type IDs for component types at compile time. Uses template
//          specialization to ensure each component type gets a unique ID.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "Types.h"
#include "../EngineExport.h"
#include <string>
#include <unordered_map>

namespace Engine {
  namespace ECS {

    /* ===================================== //
    // Automatic component type ID generation //
    // Uses type_index internally so that     //
    // the same type always gets the same ID  //
    // even across DLL/EXE module boundaries. //
    // ===================================== */
    class ENGINE_API ComponentTypeManager {
    private:
      static ComponentTypeID next_type_id;

      /// Central map from type name string -> ComponentTypeID.
      /// Uses typeid(T).name() as key instead of type_index so that
      /// keys remain valid after a DLL is unloaded and reloaded.
      /// Defined in ComponentManager.cpp — single instance in the EXE.
      static std::unordered_map<std::string, ComponentTypeID>& get_type_map();

    public:
      /*=================================================================================
      Function Brief:
      Returns a unique type ID for component type T. Each component type gets a
      unique ID that persists for the program's lifetime. Uses typeid(T) internally
      so IDs are consistent across module boundaries.

      Parameters:
      None (template parameter T determines the component type).

      Where to call it:
      Internally by the ECS when managing component storage and signatures.
      ==================================================================================*/
      template<typename T>
      static ComponentTypeID get_type_id();
    };

  } // namespace ECS
} // namespace Engine

