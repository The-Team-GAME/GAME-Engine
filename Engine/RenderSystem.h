//-----------------------------------------------------------------------------------
// file:    RenderSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines rendering systems for the ECS framework including
//          rectangle rendering, static sprite rendering, and animated sprite
//          rendering with support for transformations and colors.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "ECS/System.h"
#include "BasicComponents.h"
#include <iostream>

/*=================================================================================
Class Brief:
RectRenderSystem handles rendering of colored rectangles in the ECS framework.
It processes entities with Color, Position, and Size components and optionally
applies rotation if present.
==================================================================================*/
class RectRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the RectRenderSystem and specifies that it requires entities to have
  Color, Position, and Size components.
  ==================================================================================*/
  RectRenderSystem() {
    require_components<Components::Color, Components::Position, Components::Size>();
  }

  /*=================================================================================
  Function Brief:
  Draws a colored rectangle for the given entity. Applies optional rotation if
  the entity has a Rotation component.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* color = entity.get_component<Components::Color>();
    auto* pos = entity.get_component<Components::Position>();
    auto* size = entity.get_component<Components::Size>();

    auto rotation = entity.has_component<Components::Rotation>() ?
      *entity.get_component<Components::Rotation>() : Components::Rotation{ 0.0f };

    Engine::GRAPHICS.draw_rect({ color->X(), color->Y(), color->Z(), color->W()},
      pos->X(), pos->Y(), size->width, size->height,
      rotation.angle, Graphics::Origin::Center);
  }
};

/*=================================================================================
Class Brief:
StaticSpriteRenderSystem handles rendering of static textures in the ECS framework.
It processes entities with StaticSprite, Position, and Size components and optionally
applies rotation, scale, and color tinting if present.
==================================================================================*/
class StaticSpriteRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the StaticSpriteRenderSystem and specifies that it requires entities
  to have StaticSprite, Position, and Size components.
  ==================================================================================*/
  StaticSpriteRenderSystem() {
    require_components<Components::StaticSprite, Components::Position, Components::Size>();
  }

  /*=================================================================================
  Function Brief:
  Draws a static sprite texture for the given entity. Applies optional rotation,
  scale, and color tinting if the entity has those components.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* sprite = entity.get_component<Components::StaticSprite>();
    auto* size = entity.get_component<Components::Size>();
    auto* pos = entity.get_component<Components::Position>();

    auto rotation = entity.has_component<Components::Rotation>() ?
      *entity.get_component<Components::Rotation>() : Components::Rotation{ 0.0f };
    auto scale = entity.has_component<Components::Scale>() ?
      *entity.get_component<Components::Scale>() : Components::Scale{ 1.0f, 1.0f };
    auto color = entity.has_component<Components::Color>() ?
      *entity.get_component<Components::Color>() : Components::Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    Engine::GRAPHICS.draw_texture(sprite->texture_id, pos->X(), pos->Y(),
      size->width * scale.X(), size->height * scale.Y(),
      rotation.angle, Graphics::Origin::Center,
      { color.X(), color.Y(), color.Z(), color.W() });
  }
};

/*=================================================================================
Class Brief:
AnimatedSpriteRenderSystem handles rendering of animated sprite sheets in the ECS
framework. It updates animation time and renders the current frame with support
for rotation, scale, and color tinting.
==================================================================================*/
class AnimatedSpriteRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the AnimatedSpriteRenderSystem and specifies that it requires entities
  to have AnimatedSprite, Position, and Size components.
  ==================================================================================*/
  AnimatedSpriteRenderSystem() {
    require_components<Components::AnimatedSprite, Components::Position, Components::Size>();
  }

  /*=================================================================================
  Function Brief:
  Updates the animation time for the entity's animated sprite if it is currently
  playing. Called automatically during the update phase.

  Parameters:
  entity - Reference to the entity being updated.
  ==================================================================================*/
  void update_entity(ECS::Entity& entity) override {
    auto* sprite = entity.get_component<Components::AnimatedSprite>();

    if (sprite->isPlaying) {
      sprite->animationTime += Engine::Time::get_fixed_dt();
    }
  }

  /*=================================================================================
  Function Brief:
  Draws the current frame of an animated sprite for the given entity. Applies
  optional rotation, scale, and color tinting if the entity has those components.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* sprite = entity.get_component<Components::AnimatedSprite>();
    auto* size = entity.get_component<Components::Size>();
    auto* pos = entity.get_component<Components::Position>();

    auto rotation = entity.has_component<Components::Rotation>() ?
      *entity.get_component<Components::Rotation>() : Components::Rotation{ 0.0f };
    auto scale = entity.has_component<Components::Scale>() ?
      *entity.get_component<Components::Scale>() : Components::Scale{ 1.0f, 1.0f };
    auto color = entity.has_component<Components::Color>() ?
      *entity.get_component<Components::Color>() : Components::Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    Engine::GRAPHICS.draw_animated_sprite(sprite->spriteSheetId, pos->X(), pos->Y(),
      size->width * scale.X(), size->height * scale.Y(),
      sprite->animationTime,
      rotation.angle, Graphics::Origin::Center,
      { color.X(), color.Y(), color.Z(), color.W() });
  }
};
