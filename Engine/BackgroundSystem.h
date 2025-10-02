//-----------------------------------------------------------------------------------
// file:    BackgroundSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the BackgroundSystem which renders background colors
//          for entities with Background and Color components. Clears the screen
//          with the entity's color component.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "ECS/System.h"
#include "BasicComponents.h"

/* ============================================================== //
// Background rendering system - clears screen with solid color  //
// Could be extended for more complex background rendering       //
// ============================================================== */
class BackgroundSystem : public ECS::System {
public:
  BackgroundSystem() {
    require_components<Components::Color, Components::Background>();
  }

  void draw_entity(const ECS::Entity& entity) override {
    auto* color = entity.get_component<Components::Color>();
    Engine::GRAPHICS.clear_screen(color->X(), color->Y(), color->Z(), color->W());
  }
};


/* ================================================================ //
// Texture background rendering system - draws sprite backgrounds   //
// Works with entities that have Background and StaticSprite        //
// ================================================================ */
class TextureBackgroundSystem : public ECS::System {
public:
  TextureBackgroundSystem() {
    require_components<Components::Background, Components::StaticSprite>();
  }

  void draw_entity(const ECS::Entity& entity) override {
    auto* sprite = entity.get_component<Components::StaticSprite>();

    // Draw the background texture (no tint color in StaticSprite)
    Engine::GRAPHICS.clear_screen(sprite->texture_id);
  }
};