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

namespace Engine {
  namespace ECS
  {

    /* =================================== //
    // Automatic event type ID generation  //
    // =================================== */
    class EventTypeManager
    {
    private:
      static EventTypeID next_type_id;

    public:
      /*=================================================================================
      Function Brief:
      Returns a unique type ID for event type T. Each event type gets a unique ID
      that persists for the program's lifetime.

      Parameters:
      None (template parameter T determines the event type).

      Where to call it:
      Internally by the ECS when managing event subscriptions and dispatching.
      ==================================================================================*/
      template <typename T>
      static EventTypeID get_type_id();
    };

    /* ======================================== //
    // A wrapper for a specific event instance  //
    // ======================================== */
    struct Event
    {
      EventTypeID type_id = INVALID_EVENT_TYPE;
      std::any data;

      /*=================================================================================
      Function Brief:
      Constructs an Event with the provided event data. Automatically determines
      the event type ID from the data type.

      Parameters:
      eventData - The event data to store (forwarded).

      Where to call it:
      When creating an event to send through the event system.
      ==================================================================================*/
      template <typename T>
      Event(T&& eventData);

      /*=================================================================================
      Function Brief:
      Retrieves the event data as type T. Returns nullptr if the event type doesn't
      match.

      Parameters:
      None (template parameter T specifies the expected event data type).

      Where to call it:
      In event handlers to access the event data.
      ==================================================================================*/
      template <typename T>
      const T* get() const;
    };

  } // namespace ECS
} // namespace Engine

