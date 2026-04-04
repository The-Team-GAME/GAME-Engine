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
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "../../Engine/LevelManager.h"
#include "../../Engine/UpgradeManager.h"
#include "../../Engine/EventManager/EventManager.h"
#include "../../Engine/Events/EngineEvents.h"
#include "../../Engine/SceneManager.h"
#include "../Components/GameplayComponent.h"
#include "../GameStates/EnteringTrainCutsceneState.h"

namespace LevelTransitionStateCallbacks
{
  static constexpr const char* LEVEL_TWO_PATH = "levels/LevelTwo.json";
  static constexpr const char* PLAYER_HEARTBEAT_EVENT = "event:/SFX/PlayerHeartBeat";

  inline void to_level_two(Engine::ECS::Entity trigger_entity, Engine::ECS::Entity player_entity)
  {
    if (!trigger_entity.is_valid() || !player_entity.is_valid()) return;

    // Extra guard in case this callback is registered broadly.
    if (!player_entity.has_component<Components::Player>()) return;

    // If we're already in LevelTwo, don't re-open it / re-play the cutscene.
    // This avoids accidental infinite loops if LevelTwo contains a scene-change trigger
    // (or the player spawns overlapping one).
    {
      auto normalize = [](std::string s) {
        for (char& c : s) { if (c == '\\') c = '/'; }
        return s;
      };

      const std::string current_path = normalize(std::string(Engine::LevelManager::get_current_level_path()));
      const std::string target_rel = normalize(std::string(LEVEL_TWO_PATH));
      if (!current_path.empty())
      {
        if (current_path.ends_with(target_rel) || current_path.ends_with("LevelTwo.json"))
        {
          Log::info("[Portal] Ignoring LevelTwo transition; already in LevelTwo (current={})", current_path);
          return;
        }
      }
    }

    Log::info("[Portal] Transition triggered: trigger={}, player={} -> {}",
      trigger_entity.get_id(), player_entity.get_id(), LEVEL_TWO_PATH);

    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      std::string("Portal -> ") + LEVEL_TWO_PATH,
      Engine::Events::ToastType::Info,
      2.0f
    );

    Engine::UpgradeManager::instance().capture_level_two_entry_state();

    // Prevent repeated firing if the player keeps overlapping the volume.
    // Note: the level load will clear entities anyway; this just makes it robust.
    trigger_entity.destroy();

    // Ensure low-health heartbeat does not bleed into the level transition cutscene.
    Engine::AudioSystem::stopEvent(PLAYER_HEARTBEAT_EVENT, true);

    // Switch to the cutscene scene; it will load LevelTwo when done.
    Engine::SceneManager::load_scene("EnteringTrainCutsceneState");
  }

  // Explicitly named wrapper: portal switches into the EnteringTrainCutsceneState,
  // and that state will open LevelTwo when the cutscene ends.
  inline void portal_entering_train_cutscene(Engine::ECS::Entity trigger_entity, Engine::ECS::Entity player_entity)
  {
    to_level_two(trigger_entity, player_entity);
  }
} // namespace LevelTransitionStateCallbacks

REGISTER_FUNCTION(LevelTransitionStateCallbacks::to_level_two)
REGISTER_FUNCTION(LevelTransitionStateCallbacks::portal_entering_train_cutscene)

// Editor-friendly alias (matches the pattern used by Interactable callbacks like OldMan).
namespace {
  struct LevelTransitionLegacyAliasRegistration {
    LevelTransitionLegacyAliasRegistration() {
      using Sig = void(Engine::ECS::Entity, Engine::ECS::Entity);
       Engine::FunctionRegistry::instance().register_function<Sig>(
        "level_transition_to_level_two",
        std::function<Sig>(LevelTransitionStateCallbacks::to_level_two)
      );

       Engine::FunctionRegistry::instance().register_function<Sig>(
        "portal_entering_train_cutscene",
        std::function<Sig>(LevelTransitionStateCallbacks::portal_entering_train_cutscene)
      );
    }
  };

  inline LevelTransitionLegacyAliasRegistration s_level_transition_legacy_alias_registration;
} // namespace

