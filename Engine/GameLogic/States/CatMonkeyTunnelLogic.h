#pragma once

#include "pch.h"
#include "../../Engine/ECS/Entity.h"
#include "../../Engine/Components/SpatialComponents.h"
#include "../../Engine/Components/UtilityComponents.h"
#include "../../Engine/Time.h"
#include "../../Engine/Resources.h"
#include "CatMonkeyBossRuntime.h"

namespace CatMonkeyStateCallbacks
{
    // Tunnel logic parameters based on user request:
    // Starts at -1900 x, travels horizontally, teleports back when reaching 2750 x.
    inline void tick_tunnel_image(Engine::ECS::Entity tunnel_entity, float speed = 500.0f)
    {
        // Don't update if entity doesn't exist or isn't active
        if (!tunnel_entity.is_valid() || !tunnel_entity.is_valid()) return;

        auto* transform = tunnel_entity.get_component<Components::Transform2D>();
        if (!transform) return;

        float start_x = -1900.0f;
        float end_x = 2750.0f;

        // Move the image
        transform->position.x += speed * Engine::Time::get_fixed_dt();

        // Wrap around based on speed direction
        if (speed > 0.0f && transform->position.x >= end_x)
        {
            transform->position.x = start_x;
        }
        else if (speed < 0.0f && transform->position.x <= end_x)
        {
            transform->position.x = start_x;
        }
    }

    inline Engine::ECS::Entity find_tunnel_entity(const std::string& name_part = "Tunnel")
    {
        Engine::ECS::Entity found = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY);
        Engine::ECS::for_each_entity_with<Components::Name>(
            [&](Engine::ECS::Entity e) {
                if (found.is_valid()) return;
                auto* name = e.get_component<Components::Name>();
                if (name && name->name.find(name_part) != std::string::npos) {
                    found = e;
                }
            });
        return found;
    }

    inline void ensure_tunnel_tracked(BossRuntime& rt)
    {
        if (rt.tunnel_entity_id == Engine::ECS::INVALID_ENTITY) {
            Engine::ECS::Entity t = find_tunnel_entity();
            if (t.is_valid()) {
                rt.tunnel_entity_id = t.get_id();
            } else {
                // Not found, so instantiate the tunnel prefab!
                t = Engine::ECS::instantiate_prefab(Engine::Resources::load_prefab("prefabs/tunnel.json"));
                if (t.is_valid()) {
                    rt.tunnel_entity_id = t.get_id();
                    
                    // Optional: snap to starting position initially so it doesn't flash at a wrong spot
                    if (auto* transform = t.get_component<Components::Transform2D>()) {
                        transform->position.x = -1900.0f;
                        transform->position.y = 600.0f; 
                    }
                }
            }
        }
    }

    inline void tick_tunnel_if_active(BossRuntime& rt)
    {
        if (rt.tunnel_destroyed) {
            return;
        }
        
        ensure_tunnel_tracked(rt);
        Engine::ECS::Entity t(rt.tunnel_entity_id);
        tick_tunnel_image(t);
    }

    inline void destroy_tunnel(BossRuntime& rt)
    {
        if (rt.tunnel_entity_id != Engine::ECS::INVALID_ENTITY) {
            Engine::ECS::Entity t(rt.tunnel_entity_id);
            if (t.is_valid()) {
                t.destroy();
            }
            rt.tunnel_entity_id = Engine::ECS::INVALID_ENTITY;
        } else {
            // Failsafe: search and destroy if we lost the ID but it exists
            Engine::ECS::Entity t = find_tunnel_entity();
            if (t.is_valid()) {
                t.destroy();
            }
        }
    }
}
