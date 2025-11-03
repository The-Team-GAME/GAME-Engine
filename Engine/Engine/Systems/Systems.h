#pragma once

#include "GraphicsSystem.h"
#include "PhysicsSystem.h"
#include "UISystem.h"
#include "GizmoSystem.h"

void register_engine_systems()
{
  // Update systems
  Engine::ECS::add_system<Engine::Systems::CameraSystem>();
  Engine::ECS::add_system<Engine::Systems::Physics2DSystem>();
  Engine::ECS::add_system<Engine::Systems::AnimationUpdateSystem>();
  Engine::ECS::add_system<Engine::Systems::ClickableSystem>();
  Engine::ECS::add_system<Engine::Systems::GizmoSelectionSystem>();
  Engine::ECS::add_system<Engine::Systems::GizmoManipulationSystem>();

  // Draw systems
  Engine::ECS::add_system<Engine::Systems::StaticSpriteRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::AtlasSpriteRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::TextRenderSystem>();
}

void unregister_engine_systems()
{
  // unregister in reverse order
  Engine::ECS::remove_system<Engine::Systems::TextRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::AtlasSpriteRenderSystem>();
  Engine::ECS::remove_system<Engine::Systems::StaticSpriteRenderSystem>();

  Engine::ECS::remove_system<Engine::Systems::GizmoManipulationSystem>();
  Engine::ECS::remove_system<Engine::Systems::GizmoSelectionSystem>();
  Engine::ECS::remove_system<Engine::Systems::ClickableSystem>();
  Engine::ECS::remove_system<Engine::Systems::AnimationUpdateSystem>();
  Engine::ECS::remove_system<Engine::Systems::Physics2DSystem>();
  Engine::ECS::remove_system<Engine::Systems::CameraSystem>();
}