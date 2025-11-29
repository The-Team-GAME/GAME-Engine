//-----------------------------------------------------------------------------------
// file:    Systems.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Central header file that includes all ECS system headers and provides
//          the register_engine_systems() function to initialize all core engine
//          systems. This includes update systems (physics, collision, animation)
//          and draw systems (rendering, particles, UI) that make up the engine's
//          core functionality.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "GraphicsSystem.h"
#include "PhysicsSystem.h"
#include "CollisionSystem.h"
#include "LogicSystem.h"
#include "ParticleSystem.h"
#include "DebugDrawSystem.h"
#include "UISystem.h"
#include "GizmoSystem.h"
#include "StateMachineSystem.h"
#include "UtilitiesSystem.h"
#include "HierarchySystem.h"
#include "TrailSystem.h"

void register_engine_systems()
{
  // Update systems
  Engine::ECS::add_system<Engine::Systems::CameraSystem>();
  Engine::ECS::add_system<Engine::Systems::StateMachineSystem>(); // Update state machines first
  Engine::ECS::add_system<Engine::Systems::LogicSystem>();
  Engine::ECS::add_system<Engine::Systems::Physics2DSystem>();
  Engine::ECS::add_system<Engine::Systems::HierarchySystem>(); // Update child transforms after physics
  Engine::ECS::add_system<Engine::Systems::Collision2DSystem>();
  Engine::ECS::add_system<Engine::Systems::AnimationUpdateSystem>();
  Engine::ECS::add_system<Engine::Systems::DynamicButtonSystem>();
  Engine::ECS::add_system<Engine::Systems::ClickableSystem>();
  
  // Editor systems
#ifndef GAME_BUILD
  Engine::ECS::add_system<Engine::Systems::CameraControlSystem>();
  Engine::ECS::add_system<Engine::Systems::GizmoSelectionSystem>();
  Engine::ECS::add_system<Engine::Systems::GizmoTextSelectionSystem>();
  Engine::ECS::add_system<Engine::Systems::GizmoManipulationSystem>();
#endif

  // Effect systems
  Engine::ECS::add_system<Engine::Systems::TrailSystem>();
  Engine::ECS::add_system<Engine::Systems::FadeOutSystem>();

  // Lifetime systems (These should be added last so that they process after all other systems)
  Engine::ECS::add_system<Engine::Systems::LifetimeSystem>(); // Handle entity lifetimes
  Engine::ECS::add_system<Engine::Systems::FrameLifetimeSystem>(); // Handle frame-based entity lifetimes

  // Draw systems
  Engine::ECS::add_system<Engine::Systems::StaticSpriteRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::AtlasSpriteRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::TilemapRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::TextRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::DistortionAtlasSpriteRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::DistortionSpriteRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::ParticleSystem>();
  Engine::ECS::add_system<Engine::Systems::DebugDrawSystem>();
}

void unregister_engine_systems()
{
  // unregister in reverse order
  Engine::ECS::remove_system<Engine::Systems::DebugDrawSystem>();
  Engine::ECS::remove_system<Engine::Systems::ParticleSystem>();
  Engine::ECS::remove_system<Engine::Systems::DistortionSpriteRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::DistortionAtlasSpriteRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::TextRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::TilemapRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::AtlasSpriteRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::StaticSpriteRenderSystem>();

  Engine::ECS::remove_system<Engine::Systems::FrameLifetimeSystem>();
  Engine::ECS::remove_system<Engine::Systems::LifetimeSystem>();

  Engine::ECS::remove_system<Engine::Systems::FadeOutSystem>();
  Engine::ECS::remove_system<Engine::Systems::TrailSystem>();
  
#ifndef GAME_BUILD
  Engine::ECS::remove_system<Engine::Systems::GizmoManipulationSystem>();
  Engine::ECS::remove_system<Engine::Systems::GizmoTextSelectionSystem>();
  Engine::ECS::remove_system<Engine::Systems::GizmoSelectionSystem>();
  Engine::ECS::remove_system<Engine::Systems::CameraControlSystem>();
#endif

  Engine::ECS::remove_system<Engine::Systems::ClickableSystem>();
  Engine::ECS::remove_system<Engine::Systems::DynamicButtonSystem>();
  Engine::ECS::remove_system<Engine::Systems::AnimationUpdateSystem>();
  Engine::ECS::remove_system<Engine::Systems::Collision2DSystem>();
  Engine::ECS::remove_system<Engine::Systems::HierarchySystem>();
  Engine::ECS::remove_system<Engine::Systems::Physics2DSystem>();
  Engine::ECS::remove_system<Engine::Systems::LogicSystem>();
  Engine::ECS::remove_system<Engine::Systems::StateMachineSystem>();
  Engine::ECS::remove_system<Engine::Systems::CameraSystem>();
}