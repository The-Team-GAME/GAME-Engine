#pragma once

#include <cstdint>
#include <bitset>

namespace ECS {

  using EntityID = std::uint32_t;
  using ComponentTypeID = std::uint32_t;
  using EventTypeID = std::uint32_t;

  constexpr EntityID INVALID_ENTITY = 0;
  constexpr ComponentTypeID INVALID_COMPONENT_TYPE = 0;
  constexpr EventTypeID INVALID_EVENT_TYPE = 0;
  constexpr size_t MAX_COMPONENTS = 64;
  constexpr size_t MAX_EVENTS = 64;

  using ComponentSignature = std::bitset<MAX_COMPONENTS>;
  using EventSignature = std::bitset<MAX_EVENTS>;

} // namespace ECS