//-----------------------------------------------------------------------------------
// file:    CatMonkeyTrainTracksLogic.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file loops the train_tracks image across the screen during the Cat Monkey boss fight. It defines functions to tick the train_tracks's position each frame, find the train_tracks entity by name, ensure it's being tracked in the boss runtime, and destroy it when needed.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

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
    // Train tracks logic parameters based on user request:
    // Starts at -1900 x, travels horizontally, teleports back when reaching 2750 x.
    inline void tick_train_tracks_image(Engine::ECS::Entity train_tracks_entity, float speed = -1000.0f)
    {
        // Don't update if entity doesn't exist or isn't active
        if (!train_tracks_entity.is_valid() || !train_tracks_entity.is_valid()) return;

        auto* transform = train_tracks_entity.get_component<Components::Transform2D>();
        if (!transform) return;

        float start_x = 3000.0f;
        float end_x = -2000.0f;

        // Move the image
        transform->position.x += speed * Engine::Time::get_fixed_dt();
        transform->position.y = 900.0f; // Force y origin

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

    inline Engine::ECS::Entity find_train_tracks_entity(const std::string& name_part = "train_tracks")
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

    inline void ensure_train_tracks_tracked(BossRuntime& rt)
    {
        if (rt.train_tracks_entity_id == Engine::ECS::INVALID_ENTITY) {
            // Destroy any static train_tracks already present in the scene so we
            // don't end up with a duplicate alongside the spawned one.
            Engine::ECS::Entity existing = find_train_tracks_entity();
            if (existing.is_valid()) {
                existing.destroy();
            }

            // Always spawn a fresh instance from the prefab.
            Engine::ECS::Entity t = Engine::ECS::instantiate_prefab(Engine::Resources::load_prefab("prefabs/train_tracks.json"));
            if (t.is_valid()) {
                rt.train_tracks_entity_id = t.get_id();
                if (auto* transform = t.get_component<Components::Transform2D>()) {
                    transform->position.x = 3000.0f;
                    transform->position.y = 900.0f;
                }
            }
        }
    }

    inline void tick_train_tracks_if_active(BossRuntime& rt)
    {
        if (rt.train_tracks_destroyed) {
            return;
        }
        
        ensure_train_tracks_tracked(rt);
        Engine::ECS::Entity t(rt.train_tracks_entity_id);
        tick_train_tracks_image(t);
    }

    inline void destroy_train_tracks(BossRuntime& rt)
    {
        if (rt.train_tracks_entity_id != Engine::ECS::INVALID_ENTITY) {
            Engine::ECS::Entity t(rt.train_tracks_entity_id);
            if (t.is_valid()) {
                t.destroy();
            }
            rt.train_tracks_entity_id = Engine::ECS::INVALID_ENTITY;
        } else {
            // Failsafe: search and destroy if we lost the ID but it exists
            Engine::ECS::Entity t = find_train_tracks_entity();
            if (t.is_valid()) {
                t.destroy();
            }
        }
    }
}

