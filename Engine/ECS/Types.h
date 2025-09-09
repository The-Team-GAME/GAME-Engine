#pragma once

#include <cstdint>
#include <bitset>

namespace ECS {

  using EntityID = std::uint32_t;
  using ComponentTypeID = std::uint32_t;

  constexpr EntityID INVALID_ENTITY = 0;
  constexpr ComponentTypeID INVALID_COMPONENT_TYPE = 0;
  constexpr size_t MAX_COMPONENTS = 64;

  using ComponentSignature = std::bitset<MAX_COMPONENTS>;

} // namespace ECS