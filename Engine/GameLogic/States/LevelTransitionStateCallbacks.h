//-----------------------------------------------------------------------------------
// file:    LevelTransitionStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Level Transition state machine callback implementations (serializable)
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "../../Engine/LevelManager.h"
#include "../../Engine/EventManager/EventManager.h"
#include "../../Engine/Events/EngineEvents.h"
#include "../Components/GameplayComponent.h"

namespace LevelTransitionStateCallbacks
{
  static constexpr const char* LEVEL_TWO_PATH = "levels/LevelTwo.json";

  inline void to_level_two(Engine::ECS::Entity trigger_entity, Engine::ECS::Entity player_entity)
  {
    if (!trigger_entity.is_valid() || !player_entity.is_valid()) return;

    // Extra guard in case this callback is registered broadly.
    if (!player_entity.has_component<Components::Player>()) return;

    Log::info("[Portal] Transition triggered: trigger={}, player={} -> {}",
      trigger_entity.get_id(), player_entity.get_id(), LEVEL_TWO_PATH);

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      std::string("Portal -> ") + LEVEL_TWO_PATH,
      Engine::Events::ToastType::Info,
      2.0f
    );

    // Prevent repeated firing if the player keeps overlapping the volume.
    // Note: the level load will clear entities anyway; this just makes it robust.
    trigger_entity.destroy();

    Engine::LevelManager::open_level(LEVEL_TWO_PATH);
  }
} // namespace LevelTransitionStateCallbacks

REGISTER_FUNCTION(LevelTransitionStateCallbacks::to_level_two)

// Editor-friendly alias (matches the pattern used by Interactable callbacks like OldMan).
namespace {
  struct LevelTransitionLegacyAliasRegistration {
    LevelTransitionLegacyAliasRegistration() {
      using Sig = void(Engine::ECS::Entity, Engine::ECS::Entity);
       Engine::FunctionRegistry::instance().register_function<Sig>(
        "level_transition_to_level_two",
        std::function<Sig>(LevelTransitionStateCallbacks::to_level_two)
      );
    }
  };

  inline LevelTransitionLegacyAliasRegistration s_level_transition_legacy_alias_registration;
} // namespace
