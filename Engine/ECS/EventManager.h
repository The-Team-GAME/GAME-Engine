//-----------------------------------------------------------------------------------
// file:    EventManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the event system for the ECS. Includes automatic event
//          type ID generation and an Event wrapper class for type-safe event data
//          storage and retrieval using std::any.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include <any>
#include <functional>
#include <unordered_map>
#include <vector>

namespace ECS
{

  /* =================================== //
  // Automatic event type ID generation  //
  // =================================== */
  class EventTypeManager
  {
  private:
    static EventTypeID nextTypeId;

  public:
    template <typename T>
    static EventTypeID getTypeId();
  };

  /* ======================================== //
  // A wrapper for a specific event instance  //
  // ======================================== */
  struct Event
  {
    EventTypeID typeId = INVALID_EVENT_TYPE;
    std::any data;

    template <typename T>
    Event(T&& eventData);

    template <typename T>
    const T* get() const;
  };

} // namespace ECS

