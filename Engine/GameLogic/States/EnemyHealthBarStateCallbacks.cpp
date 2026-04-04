//-----------------------------------------------------------------------------------
// file:    EnemyHealthBarStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Enemy Health Bar state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "../Components/EnemyHealthComponent.h"
#include "../../Engine/Components/HierarchyComponents.h"

#include <unordered_map>

namespace EnemyHealthBarStateCallbacks
{
  inline std::unordered_map<Engine::ECS::EntityID, int>& get_recorded_hp_map() {
    static std::unordered_map<Engine::ECS::EntityID, int> recorded_hp;
    return recorded_hp;
  }

  inline Engine::ECS::Entity get_parent_enemy(Engine::ECS::Entity entity) {
    auto* parent = entity.get_component<Components::Parent>();
    if (!parent || parent->parent_id == Engine::ECS::INVALID_ENTITY) {
      return {};
    }

    Engine::ECS::Entity parent_entity(parent->parent_id);
    if (!parent_entity.is_valid()) {
      return {};
    }

    return parent_entity;
  }

  inline Components::EnemyHealth* get_parent_enemy_health(Engine::ECS::Entity entity) {
    auto parent_entity = get_parent_enemy(entity);
    if (!parent_entity.is_valid()) {
      return nullptr;
    }

    return parent_entity.get_component<Components::EnemyHealth>();
  }

  inline void set_bar_alpha(Engine::ECS::Entity entity, float alpha) {
    if (auto* color = entity.get_component<Components::Color>()) {
      color->a = alpha;
    }
  }

  inline void restart_bar_animation(Engine::ECS::Entity entity) {
    if (auto* animation = entity.get_component<Components::Animation>()) {
      animation->startTime = Engine::Time::get_time();
    }

    if (auto* atlas = entity.get_component<Components::TextureAtlas>()) {
      atlas->drawIndex = 0;
    }
  }

  inline void remember_current_hp(Engine::ECS::Entity entity, int hp) {
    get_recorded_hp_map()[entity.get_id()] = hp;
  }

  inline int get_recorded_hp(Engine::ECS::Entity entity) {
    auto& recorded_hp = get_recorded_hp_map();
    auto it = recorded_hp.find(entity.get_id());
    return (it == recorded_hp.end()) ? -1 : it->second;
  }

  inline void clear_recorded_hp(Engine::ECS::Entity entity) {
    get_recorded_hp_map().erase(entity.get_id());
  }

  inline bool detect_damage_and_remember(Engine::ECS::Entity entity, const Components::EnemyHealth& health) {
    const int previous_hp = get_recorded_hp(entity);
    remember_current_hp(entity, health.current_hp);
    return previous_hp >= 0 && health.current_hp < previous_hp;
  }

  inline const char* resolve_health_state(const Components::EnemyHealth& health) {
    if (health.is_dead()) {
      return "Death";
    }

    if (health.current_hp >= health.max_hp) {
      return "Blank";
    }

    const float ratio = (health.max_hp > 0)
      ? static_cast<float>(health.current_hp) / static_cast<float>(health.max_hp)
      : 0.0f;

    if (ratio > (4.0f / 5.0f)) return "1";
    if (ratio > (3.0f / 5.0f)) return "2";
    if (ratio > (2.0f / 5.0f)) return "3";
    if (ratio > (1.0f / 5.0f)) return "4";
    return "5";
  }

  inline void update_stable_state(Engine::ECS::Entity entity) {
    auto* health = get_parent_enemy_health(entity);
    auto* state_machine = entity.get_component<Components::StateMachine>();
    if (!state_machine) {
      return;
    }

    if (!health) {
      state_machine->change_state("Death");
      return;
    }

    set_bar_alpha(entity, 1.0f);

    if (health->is_dead()) {
      state_machine->change_state("Death");
      return;
    }

    if (detect_damage_and_remember(entity, *health)) {
      state_machine->change_state("Damaged");
      return;
    }

    const char* desired_state = resolve_health_state(*health);
    if (state_machine->get_current_state() != desired_state) {
      state_machine->change_state(desired_state);
    }
  }

  inline void blank_on_enter(Engine::ECS::Entity entity) {
    set_bar_alpha(entity, 0.0f);

    if (auto* health = get_parent_enemy_health(entity)) {
      remember_current_hp(entity, health->current_hp);
    }
  }

  inline void blank_on_update(Engine::ECS::Entity entity) {
    auto* health = get_parent_enemy_health(entity);
    auto* state_machine = entity.get_component<Components::StateMachine>();
    if (!state_machine) {
      return;
    }

    if (!health) {
      state_machine->change_state("Death");
      return;
    }

    if (detect_damage_and_remember(entity, *health)) {
      set_bar_alpha(entity, 1.0f);
      state_machine->change_state("Damaged");
    }
  }

  inline void damaged_on_enter(Engine::ECS::Entity entity) {
    set_bar_alpha(entity, 1.0f);

    if (auto* health = get_parent_enemy_health(entity)) {
      remember_current_hp(entity, health->current_hp);
    }

    restart_bar_animation(entity);
  }

  inline void damaged_on_update(Engine::ECS::Entity entity) {
    auto* health = get_parent_enemy_health(entity);
    auto* state_machine = entity.get_component<Components::StateMachine>();
    if (!state_machine) {
      return;
    }

    if (!health) {
      state_machine->change_state("Death");
      return;
    }

    set_bar_alpha(entity, 1.0f);

    if (health->is_dead()) {
      state_machine->change_state("Death");
      return;
    }

    if (detect_damage_and_remember(entity, *health)) {
      restart_bar_animation(entity);
      return;
    }

    auto* animation = entity.get_component<Components::Animation>();
    if (!animation || animation->has_ended()) {
      state_machine->change_state(resolve_health_state(*health));
    }
  }

  inline void stable_on_enter(Engine::ECS::Entity entity) {
    set_bar_alpha(entity, 1.0f);

    if (auto* health = get_parent_enemy_health(entity)) {
      remember_current_hp(entity, health->current_hp);
    }

  }

  inline void stable_on_update(Engine::ECS::Entity entity) {
    update_stable_state(entity);
  }

  inline void death_on_enter(Engine::ECS::Entity entity) {
    set_bar_alpha(entity, 1.0f);
    restart_bar_animation(entity);
    clear_recorded_hp(entity);
  }

  inline void death_on_update(Engine::ECS::Entity entity) {
    set_bar_alpha(entity, 1.0f);
  }
}

REGISTER_STATE_MACHINE_BLUEPRINT(EnemyHealthBar, "Blank",
  STATE("Blank", EnemyHealthBarStateCallbacks::blank_on_enter, EnemyHealthBarStateCallbacks::blank_on_update, NO_CALLBACK)
  STATE("Damaged", EnemyHealthBarStateCallbacks::damaged_on_enter, EnemyHealthBarStateCallbacks::damaged_on_update, NO_CALLBACK)
  STATE("1", EnemyHealthBarStateCallbacks::stable_on_enter, EnemyHealthBarStateCallbacks::stable_on_update, NO_CALLBACK)
  STATE("2", EnemyHealthBarStateCallbacks::stable_on_enter, EnemyHealthBarStateCallbacks::stable_on_update, NO_CALLBACK)
  STATE("3", EnemyHealthBarStateCallbacks::stable_on_enter, EnemyHealthBarStateCallbacks::stable_on_update, NO_CALLBACK)
  STATE("4", EnemyHealthBarStateCallbacks::stable_on_enter, EnemyHealthBarStateCallbacks::stable_on_update, NO_CALLBACK)
  STATE("5", EnemyHealthBarStateCallbacks::stable_on_enter, EnemyHealthBarStateCallbacks::stable_on_update, NO_CALLBACK)
  STATE("Death", EnemyHealthBarStateCallbacks::death_on_enter, EnemyHealthBarStateCallbacks::death_on_update, NO_CALLBACK)
)