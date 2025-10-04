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
#include "Graphics/Components.h"
#include "Graphics/Renderer.h"
#include "Graphics/Resource.h"
#include "Time.h"
#include <iostream>

/*=================================================================================
Class Brief:
RectRenderSystem handles rendering of colored rectangles in the ECS framework.
It processes entities with Transform2D and uses the unit_rect texture with color tinting.
==================================================================================*/
class RectRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the RectRenderSystem and specifies that it requires entities to have
  Transform2D component.
  ==================================================================================*/
  RectRenderSystem() {
    require_components<Components::Transform2D>();
  }

  /*=================================================================================
  Function Brief:
  Draws a colored rectangle for the given entity using the unit_rect texture.
  Applies color tinting, position, rotation, and scale from the Transform2D component.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* transform = entity.get_component<Components::Transform2D>();

    // Check if entity has a Color component, otherwise use white
    Math::Vec4D color(255.0f, 255.0f, 255.0f, 255.0f);
    auto* color_comp = entity.get_component<Components::Color>();
    if (color_comp) {
      color = color_comp->color;
    }

    // Get the unit rect texture (1x1 white pixel)
    auto texture = Engine::Resources::get_unit_rect_texture();

    // Draw the texture with transform and color
    Engine::Graphics::draw_texture(
      texture,
      transform->position,
      transform->scale,
      transform->rotation,
      color
    );
  }
};

/*=================================================================================
Class Brief:
TextureRenderSystem handles rendering of static textures in the ECS framework.
It processes entities with Texture and Transform2D components.
==================================================================================*/
class TextureRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the TextureRenderSystem and specifies that it requires entities
  to have Texture and Transform2D components.
  ==================================================================================*/
  TextureRenderSystem() {
    require_components<Components::Texture, Components::Transform2D>();
  }

  /*=================================================================================
  Function Brief:
  Draws a texture for the given entity. Applies position, rotation, and scale
  from Transform2D component.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* texture_comp = entity.get_component<Components::Texture>();
    auto* transform = entity.get_component<Components::Transform2D>();

    if (!texture_comp->texture_handle) {
      return; // Skip if texture is invalid
    }

    // Check if entity has a Color component, otherwise use white
    Math::Vec4D color(255.0f, 255.0f, 255.0f, 255.0f);
    auto* color_comp = entity.get_component<Components::Color>();
    if (color_comp) {
      color = color_comp->color;
    }

    // Draw the texture
    Engine::Graphics::draw_texture(
      texture_comp->texture_handle,
      transform->position,
      transform->scale,
      transform->rotation,
      color
    );
  }
};

/*=================================================================================
Class Brief:
TextureAtlasRenderSystem handles rendering of texture atlases (sprite sheets)
in the ECS framework. It processes entities with Texture, TextureAtlas, and
Transform2D components.
==================================================================================*/
class TextureAtlasRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the TextureAtlasRenderSystem and specifies that it requires entities
  to have Texture, TextureAtlas, and Transform2D components.
  ==================================================================================*/
  TextureAtlasRenderSystem() {
    require_components<Components::Texture, Components::TextureAtlas, Components::Transform2D>();
  }

  /*=================================================================================
  Function Brief:
  Draws a specific subtexture from a texture atlas for the given entity.
  Uses the drawIndex to select which subtexture to render.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* texture_comp = entity.get_component<Components::Texture>();
    auto* atlas = entity.get_component<Components::TextureAtlas>();
    auto* transform = entity.get_component<Components::Transform2D>();

    if (!texture_comp->texture_handle || atlas->subTextures.empty()) {
      return;
    }

    // Clamp drawIndex to valid range
    size_t index = atlas->drawIndex % atlas->subTextures.size();

    // Check if entity has a Color component, otherwise use white
    Math::Vec4D color(255.0f, 255.0f, 255.0f, 255.0f);
    auto* color_comp = entity.get_component<Components::Color>();
    if (color_comp) {
      color = color_comp->color;
    }

    // Draw the subtexture
    Engine::Graphics::draw_subtexture(
      texture_comp->texture_handle,
      atlas->subTextures[index].tex_coords,
      transform->position,
      transform->scale,
      transform->rotation,
      color
    );
  }
};

/*=================================================================================
Class Brief:
AnimatedSpriteRenderSystem handles rendering of animated sprite sheets in the ECS
framework. It updates animation time and renders the current frame based on the
animation state.
==================================================================================*/
class AnimatedSpriteRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the AnimatedSpriteRenderSystem and specifies that it requires entities
  to have Texture, TextureAtlas, Animation, and Transform2D components.
  ==================================================================================*/
  AnimatedSpriteRenderSystem() {
    require_components<Components::Texture, Components::TextureAtlas, Components::Animation, Components::Transform2D>();
  }

  /*=================================================================================
  Function Brief:
  Updates the animation state for the entity. Calculates which frame should be
  displayed based on elapsed time. Called automatically during the update phase.

  Parameters:
  entity - Reference to the entity being updated.
  ==================================================================================*/
  void update_entity(ECS::Entity& entity) override {
    auto* animation = entity.get_component<Components::Animation>();
    auto* atlas = entity.get_component<Components::TextureAtlas>();

    if (animation->frames.empty()) {
      return;
    }

    // Update animation time
    float current_time = Engine::Time::get_time() - animation->startTime;

    // Loop the animation
    if (current_time >= animation->totalDuration) {
      current_time = fmod(current_time, animation->totalDuration);
      animation->startTime = Engine::Time::get_time() - current_time;
    }

    // Find which frame we should be on
    for (size_t i = 0; i < animation->frames.size(); ++i) {
      if (current_time >= animation->frames[i].before_time &&
        current_time < animation->frames[i].before_time + animation->frames[i].duration) {
        atlas->drawIndex = i;
        break;
      }
    }
  }

  /*=================================================================================
  Function Brief:
  Draws the current frame of an animated sprite for the given entity.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* texture_comp = entity.get_component<Components::Texture>();
    auto* atlas = entity.get_component<Components::TextureAtlas>();
    auto* transform = entity.get_component<Components::Transform2D>();

    if (!texture_comp->texture_handle || atlas->subTextures.empty()) {
      return;
    }

    // Clamp drawIndex to valid range
    size_t index = atlas->drawIndex % atlas->subTextures.size();

    // Check if entity has a Color component, otherwise use white
    Math::Vec4D color(255.0f, 255.0f, 255.0f, 255.0f);
    if (entity.has_component< Components::Color>())
    {
      auto* color_comp = entity.get_component<Components::Color>();
      color = color_comp->color;
    }

    // Draw the current animation frame
    Engine::Graphics::draw_subtexture(
      texture_comp->texture_handle,
      atlas->subTextures[index].tex_coords,
      transform->position,
      transform->scale,
      transform->rotation,
      color
    );
  }
};

/*=================================================================================
Class Brief:
TextRenderSystem handles rendering of text in the ECS framework.
It processes entities with Font, Text, and Transform2D components.
==================================================================================*/
class TextRenderSystem : public ECS::System {
public:
  /*=================================================================================
  Function Brief:
  Constructs the TextRenderSystem and specifies that it requires entities
  to have Font, Text, and Transform2D components.
  ==================================================================================*/
  TextRenderSystem() {
    require_components<Components::Font, Components::Text, Components::Transform2D>();
  }

  /*=================================================================================
  Function Brief:
  Draws text for the given entity using the specified font.

  Parameters:
  entity - Reference to the entity being rendered.
  ==================================================================================*/
  void draw_entity(const ECS::Entity& entity) override {
    auto* font_comp = entity.get_component<Components::Font>();
    auto* text = entity.get_component<Components::Text>();
    auto* transform = entity.get_component<Components::Transform2D>();

    if (!font_comp->font_handle || text->content.empty()) {
      return;
    }

    // Check if entity has a Color component, otherwise use white
    Math::Vec4D color(255.0f, 255.0f, 255.0f, 255.0f);
    auto* color_comp = entity.get_component<Components::Color>();
    if (color_comp) {
      color = color_comp->color;
    }

    // Draw the text
    // Use transform->scale.x as the font scale factor
    Engine::Graphics::draw_text(
      font_comp->font_handle,
      text->content,
      transform->position,
      transform->scale.x,
      color
    );
  }
};
