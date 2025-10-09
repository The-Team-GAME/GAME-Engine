#pragma once

#include "GraphicsSystem.h"
#include "PhysicsSystem.h"

void register_engine_systems()
{
  // Update systems
  Engine::ECS::add_system<Engine::Systems::Transform2DSystem>();
  Engine::ECS::add_system<Engine::Systems::AnimationUpdateSystem>();

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

  Engine::ECS::remove_system<Engine::Systems::AnimationUpdateSystem>();
  Engine::ECS::remove_system<Engine::Systems::Transform2DSystem>();
}