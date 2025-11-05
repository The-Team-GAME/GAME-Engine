#pragma once
#include <sol/sol.hpp>
#include <string>
#include <cstdint>

namespace Engine
{
  namespace GameAPI
  {
    using EntityId = uint32_t;

    struct EntityProxy
    {
      EntityId id;
    };

    // Register Game API into Sol state
    void register_api(sol::state& lua);

    // Low-level wrappers (call your ECS/Resources APIs)
    EntityId create_entity();
    void destroy_entity(EntityId id);

    void add_transform(EntityId id, float x, float y, float rotation = 0.0f, float sx = 1.0f, float sy = 1.0f);
    void add_texture(EntityId id, const std::string& path);
    void add_color(EntityId id, float r, float g, float b, float a);
    void add_rigidbody(EntityId id, float mass = 1.0f);
    void add_tag(EntityId id, const std::string& tag);
    
    // Convenience: create from a Lua table
    EntityId create_from_table(const sol::table& t);
  }
  
}