//-----------------------------------------------------------------------------------
// file:    GameAPI.cpp
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Implementation of the Lua-C++ binding layer for the game engine.
//          Provides component reflection system, entity manipulation, physics
//          integration, and resource management. Handles registration of engine
//          functionality to Lua state and supports dynamic component access.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "GameAPI.h"
#include "../Logger.h"
#include <sol/sol.hpp>
#include <type_traits>

// ...existing code...

// Adapt these includes to your project layout
#include "../ECS/ECS.h"           // Engine::ECS::create_entity(), get_entity_by_id(), destroy_entity_by_id()
#include "../Components/Components.h" // Transform2D, Texture, Color, RigidBody
#include "../MonkeyEngine.h"
namespace Engine
{
  namespace GameAPI
  {

    // //This is a temporary solution for game obj behaviour -Ryanh
    // struct TransformData {
    //     float x, y;
    //     float rotation;
    //     float sx, sy;
    // };

    // TransformData get_transform(EntityId id) {
    //     auto ent = Engine::ECS::get_entity(id);
    //     TransformData t{0,0,0,1,1};
    //     if (auto* transform = ent.get_component<Components::Transform2D>()) {
    //         t.x = transform->position.x;
    //         t.y = transform->position.y;
    //         t.rotation = transform->rotation;
    //         t.sx = transform->scale.x;
    //         t.sy = transform->scale.y;
    //     }
    //     return t;
    // }

    // void set_transform(EntityId id, const TransformData& t) {
    //     auto ent = Engine::ECS::get_entity(id);
    //     if (auto* transform = ent.get_component<Components::Transform2D>()) {
    //         transform->position.x = t.x;
    //         transform->position.y = t.y;
    //         transform->rotation = t.rotation;
    //         transform->scale.x = t.sx;
    //         transform->scale.y = t.sy;
    //     }
    // }
    //temp ends here

    // More complicated getter and setter - Ryanh
    // Generic component getter/setter template

    // Add these reflection helpers
    template<typename T>
    sol::object reflect_get(T& component, sol::object key, sol::state_view lua) {
      if (!key.is<std::string>()) return sol::nil;
      std::string field = key.as<std::string>();

      if constexpr (std::is_same_v<T, Components::Transform2D>) {
        if (field == "x") return sol::make_object(lua, component.position.x);
        if (field == "y") return sol::make_object(lua, component.position.y);
        if (field == "rotation") return sol::make_object(lua, component.rotation);
        if (field == "sx") return sol::make_object(lua, component.scale.x);
        if (field == "sy") return sol::make_object(lua, component.scale.y);
      }
      else if constexpr (std::is_same_v<T, Components::Color>) {
        if (field == "r") return sol::make_object(lua, component.r);
        if (field == "g") return sol::make_object(lua, component.g);
        if (field == "b") return sol::make_object(lua, component.b);
        if (field == "a") return sol::make_object(lua, component.a);
      }
      return sol::nil;
    }

    template<typename T>
    void reflect_set(T& component, sol::object key, sol::object value) {
      std::string field = key.as<std::string>();

      if constexpr (std::is_same_v<T, Components::Transform2D>) {
        if (field == "x") component.position.x = value.as<f32>();
        if (field == "y") component.position.y = value.as<f32>();
        if (field == "rotation") component.rotation = value.as<f32>();
        if (field == "sx") component.scale.x = value.as<f32>();
        if (field == "sy") component.scale.y = value.as<f32>();
      }
      else if constexpr (std::is_same_v<T, Components::Color>) {
        if (field == "r") component.r = value.as<f32>();
        if (field == "g") component.g = value.as<f32>();
        if (field == "b") component.b = value.as<f32>();
        if (field == "a") component.a = value.as<f32>();
      }
    }

    template<typename T>
    T* get_component(EntityId id) {
      auto ent = Engine::ECS::get_entity(id);
      return ent.get_component<T>();
    }

    // Helper to register component type with Lua
    template<typename T>
void register_component_type(sol::state& lua, const std::string& name) {
    sol::state_view sv(lua);
    lua.new_usertype<T>(name,
        sol::no_constructor,  // Don't try to construct components directly from Lua
        sol::meta_function::index, [sv](T& component, sol::object key) -> sol::object {
            return reflect_get<T>(component, key, sv);
        },
        sol::meta_function::new_index, [](T& component, sol::object key, sol::object value) {
            reflect_set<T>(component, key, value);
        }
    );
}

     std::vector<EntityProxy> get_all_entities() {
        std::vector<EntityProxy> proxies;
        auto entities = Engine::ECS::get_all_entities();
        for (auto& ent : entities) {
            EntityProxy p;
            p.id = ent.get_id();
            proxies.push_back(p);
        }
        return proxies;
    }

    // Create entity and return its id
    EntityId create_entity()
    {
      // Engine::ECS::create_entity() must return an entity object with get_id()
      auto e = Engine::ECS::create_entity();
      return static_cast<EntityId>(e.get_id());
    }

    void destroy_entity(EntityId id)
    {
      // Ensure you have a wrapper in ECS that destroys by id
      Engine::ECS::destroy_entity(id);
    }

    template<typename Entity>
    void add_tag_helper(Entity ent, const std::string& tag) {

      //**temp fix 
      if (tag == "MainMenuState") {
        ent.add_component<Components::Tag<"MainMenuState">>();
      }
      else if (tag == "LuaState") {
        ent.add_component<Components::Tag<"LuaState">>();
      }
      else if (tag == "PhysicsTestState") {
        ent.add_component<Components::Tag<"PhysicsTestState">>();
      }
      else if (tag == "BehaviourState") {
        ent.add_component<Components::Tag<"BehaviourState">>();
      }
      else {
        Log::error("[GameAPI] Unknown tag type: {}", tag);
      }
    }

    void add_transform(EntityId id, f32 x, f32 y, f32 rotation, f32 sx, f32 sy)
    {
      auto ent = Engine::ECS::get_entity(id); // adapt name if different
      ent.add_component<Components::Transform2D>(x, y, rotation, sx, sy);
    }

    void add_texture(EntityId id, const std::string& path)
    {
      auto tex = Engine::Resources::load_texture(path);
      auto ent = Engine::ECS::get_entity(id);
      ent.add_component<Components::Texture>(tex);
    }

		// Overloaded version to add a default rectangle texture
    void add_texture_quad(EntityId id)
    {
      auto ent = Engine::ECS::get_entity(id);
      ent.add_component<Components::Texture>(Engine::Resources::get_rect_texture());
    }

    void add_color(EntityId id, f32 r, f32 g, f32 b, f32 a)
    {
      auto ent = Engine::ECS::get_entity(id);
      ent.add_component<Components::Color>(Math::Vec4D(r, g, b, a));
    }

    void add_rigidbody(EntityId id, bool is_kinematic, bool is_gravity)
    {
      auto ent = Engine::ECS::get_entity(id);
      ent.add_component<Components::RigidBody>(is_kinematic, is_gravity);
    }

    void add_box_collider(EntityId id, f32 width, f32 height, bool is_static)
    {
      auto ent = Engine::ECS::get_entity(id);
      ent.add_component<Components::BoxCollider>(width, height, is_static);
		}

    void add_tag(EntityId id, const std::string& tag) {
      auto ent = Engine::ECS::get_entity(id);
      add_tag_helper(ent, tag);
    }

		// This one creates from a table 
    // which means that u can serialize this easily <3 - Ryanh
    EntityId create_from_table(const sol::table& t)
    {
      EntityId id = create_entity();
      // components expected as an array of tables with _type field
      if (t["components"].valid()) {
        sol::table comps = t["components"];
        for (auto& kv : comps) {
          sol::object val = kv.second;
          if (!val.is<sol::table>()) continue;
          sol::table ct = val.as<sol::table>();
          std::string type = ct.get<std::string>("_type");

          if (type == "Transform") {
            f32 x = ct.get_or("x", 0.0f);
            f32 y = ct.get_or("y", 0.0f);
            f32 rot = ct.get_or("rotation", 0.0f);
            f32 sx = ct.get_or("sx", 1.0f);
            f32 sy = ct.get_or("sy", 1.0f);
            add_transform(id, x, y, rot, sx, sy);
          }
          else if (type == "Texture") {
            std::string path = ct.get_or("path", std::string{});
            add_texture(id, path);
          }
          else if (type == "Color") {
            f32 r = ct.get_or("r", 1.0f);
            f32 g = ct.get_or("g", 1.0f);
            f32 b = ct.get_or("b", 1.0f);
            f32 a = ct.get_or("a", 1.0f);
            add_color(id, r, g, b, a);
          }
          else if (type == "RigidBody") {
            //f32 mass = ct.get_or("mass", 1.0f);
            bool kinematic = ct.get_or("isKinematic", true);
            bool gravity = ct.get_or("isGravity", true);
            add_rigidbody(id, kinematic, gravity);
          }
          else if (type == "BoxCollider") {
            //f32 mass = ct.get_or("mass", 1.0f);
            f32 width = ct.get_or("width", 0.0f);
            f32 height = ct.get_or("height", 0.0f);
						bool isStatic = ct.get_or("isStatic", true);
            add_box_collider(id, width, height, isStatic);
          }
          else if (type == "Tag") {
            std::string tag = ct.get_or("tag", std::string{});
            add_tag(id, tag);
          }
          else {
            Log::error("[GameAPI] Unknown component type: {}", type);
          }
        }
      }
      return id;
    }

    void register_api(sol::state& lua)
    {
      sol::table game = lua.create_table();

      // a more generic way to register component types
      // Register component types
      // Register component types first
      register_component_type<Components::Transform2D>(lua, "Transform2D");
      register_component_type<Components::Color>(lua, "Color");
      register_component_type<Components::Texture>(lua, "Texture");
      register_component_type<Components::RigidBody>(lua, "RigidBody");
      register_component_type<Components::BoxCollider>(lua, "BoxCollider");
			//register_component_type<Components::Tag<"MainMenuState">>(lua, "Tag_MainMenuState");

      game.set_function("create_entity", &create_entity);
      game.set_function("destroy_entity", &destroy_entity);
      game.set_function("create_from_table", [](sol::table t) { return create_from_table(t); });

      // old stuff 
      game.set_function("add_transform", &add_transform);
      game.set_function("add_texture", &add_texture);
      game.set_function("add_texture_quad", &add_texture_quad);
      game.set_function("add_color", &add_color);
      game.set_function("add_rigidbody", &add_rigidbody);
			game.set_function("add_box_collider", &add_box_collider);
      game.set_function("add_tag", &add_tag);
      
      // Add input checking functions
      game.set_function("is_key_held", [](const std::string& key) {
        if (key == "W") return Engine::Input::is_held(Engine::KeyCode::W);
        if (key == "A") return Engine::Input::is_held(Engine::KeyCode::A);
        if (key == "S") return Engine::Input::is_held(Engine::KeyCode::S);
        if (key == "D") return Engine::Input::is_held(Engine::KeyCode::D);
        return false;
        });

      //Temp transform stuff (might change in future) - Ryan how
            // Register TransformData type
      // lua.new_usertype<TransformData>("TransformData",
      //      "x", &TransformData::x,
      //      "y", &TransformData::y,
      //      "rotation", &TransformData::rotation,
      //      "sx", &TransformData::sx,
      //      "sy", &TransformData::sy
      // );

      // // Add transform functions to Game table
      // game.set_function("get_transform", &get_transform);
      // game.set_function("set_transform", &set_transform);



      // proxy userdata
      lua.new_usertype<EntityProxy>("Entity",
        "id", &EntityProxy::id,

        // Generic component getter 
        "get_component", [&lua](EntityProxy& p, const std::string& type) -> sol::object {
          if (type == "Transform2D")
            return sol::make_object(lua, get_component<Components::Transform2D>(p.id));
          if (type == "Color")
            return sol::make_object(lua, get_component<Components::Color>(p.id));
          if (type == "Texture")
            return sol::make_object(lua, get_component<Components::Texture>(p.id));
          if (type == "RigidBody")
            return sol::make_object(lua, get_component<Components::RigidBody>(p.id));
          if (type == "BoxCollider")
            return sol::make_object(lua, get_component<Components::BoxCollider>(p.id));
          // if (type == "Tag")
          //   return sol::make_object(lua, add_tag(p.id));
          return sol::nil;
        },

        // Legacy add_* methods for backward compatibility
        "add_transform", [](EntityProxy& p, f32 x, f32 y, f32 rot, f32 sx, f32 sy) {
          add_transform(p.id, x, y, rot, sx, sy);
        },
        "add_color", [](EntityProxy& p, f32 r, f32 g, f32 b, f32 a) {
          add_color(p.id, r, g, b, a);
        },
        "add_texture", [](EntityProxy& p, const std::string& path) {
          add_texture(p.id, path);
        },
        "add_texture_quad", [](EntityProxy& p) {
          add_texture_quad(p.id);
        },
        "add_rigidbody", [](EntityProxy& p, bool is_kinematic, bool is_gravity) {
          add_rigidbody(p.id, is_kinematic, is_gravity);
        },
        "add_box_collider", [](EntityProxy& p, f32 width, f32 height, bool is_trigger) {
          add_box_collider(p.id, width, height, is_trigger);
        },
        "add_tag", [](EntityProxy& p, const std::string& tag) {
          add_tag(p.id, tag);
        }
        
        //temp stuff
        // "get_transform", [](EntityProxy& p) { return get_transform(p.id); },
        // "set_transform", [](EntityProxy& p, const TransformData& t) { set_transform(p.id, t); }
      );

      game.set_function("create_entity", []() {
        EntityProxy p; p.id = create_entity(); return p;
        });


      game.set_function("get_all_entities", &get_all_entities);


      game.set_function("get_entity_proxy", [](EntityId id) {
        EntityProxy p; p.id = id; return p;
        });

      lua["Game"] = game;
      Log::info("[GameAPI] Registered Game API in Lua");
    }
  }
}