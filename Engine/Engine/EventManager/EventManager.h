//-----------------------------------------------------------------------------------
// file:    EventManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the event system. Includes automatic event
//          type ID generation and an Event wrapper class for type-safe event data
//          storage and retrieval using std::any.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <any>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <bitset>
#include "../TypeDef.h"

namespace Engine {
  namespace ECS {
    class System;
  }

  namespace EventManager
  {
    using EventTypeID = u32;
    inline constexpr size_t MAX_EVENTS = 64;
    using EventSignature = std::bitset<MAX_EVENTS>;
    inline constexpr EventTypeID INVALID_EVENT_TYPE = 0;

    /* =================================== //
    // Automatic event type ID generation  //
    // =================================== */
    class EventTypeManager
    {
    private:
      static EventTypeID next_type_id;

    public:
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

      template <typename T>
      Event(T&& eventData);

      template <typename T>
      const T* get() const;
    };

    /* ================= //
    // Event management  //
    // ================= */
    template <typename T, typename... Args>
    void send_event(Args&&... args);

    template <typename T>
    void add_event_listener(std::function<void(const T&)> handler);

    template <typename T>
    void add_event_listener(std::function<void(const Event&)> handler);

    void remove_event_listener(EventTypeID eventType);

    template <typename T>
    void remove_event_listener();

    void update_events(const std::vector<std::unique_ptr<ECS::System>>& systems);

    /* =============== //
    // Internal state  //
    // =============== */
    namespace Internal
    {
      extern std::vector<Event> event_queue;
      extern std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>> event_handlers;
    }

  } // namespace EventManager
} // namespace Engine

#include "EventManager.inl"