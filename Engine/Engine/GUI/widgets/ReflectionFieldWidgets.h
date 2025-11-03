//-----------------------------------------------------------------------------------
// file:    ReflectionFieldWidgets.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Reusable ImGui widgets for rendering reflection-based component fields.
//          Provides specialized rendering for common types like Color, Texture, Font,
//          and generic rendering for primitives, objects, and arrays.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <functional>
#include "../../Reflection/ReflectionLibrary.h"
#include "../../ECS/ECS.h"

namespace ImGuiWidgets {

  /*=================================================================================
  Function Brief:
  Renders a color picker widget for Components::Color fields.

  Parameters:
  field_value - Reference to the GenericValue containing RGBA color data
  modified    - [out] Set to true if the color was modified by user input

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for color components.
  ==================================================================================*/
  bool render_color_field(ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders a texture picker widget with preview for Components::Texture fields.

  Parameters:
  field_value - Reference to the GenericValue containing texture path data
  modified    - [out] Set to true if the texture was modified by user input

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for texture components.
  ==================================================================================*/
  bool render_texture_field(ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders a font picker widget with size input for Components::Font fields.

  Parameters:
  field_value - Reference to the GenericValue containing font path and size data
  modified    - [out] Set to true if the font was modified by user input

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for font components.
  ==================================================================================*/
  bool render_font_field(ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders a texture atlas preview widget for Components::TextureAtlas fields.
  Only works when an entity is provided (requires Texture component).

  Parameters:
  entity - Optional entity to get the Texture component from

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for texture atlas components.
  ==================================================================================*/
  bool render_texture_atlas_field(Engine::ECS::Entity entity);

  /*=================================================================================
  Function Brief:
  Renders specialized UI fields for specific component types (e.g., Color, Texture,
  Font). Returns true if the field was handled and rendered, false otherwise.

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input
  entity       - Optional entity for context-dependent rendering (e.g., TextureAtlas)

  Returns:
  true if a specialized renderer handled the field, false to fall back to generic.

  Where to call it:
  Before attempting generic field rendering.
  ==================================================================================*/
  bool render_specialized_field(
    const std::string& display_name,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY)
  );

  /*=================================================================================
  Function Brief:
  Renders generic UI fields based on the field value type (bool, int, float, string,
  object, array).

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input
  entity       - Optional entity for context-dependent rendering

  Returns:
  true if the field was successfully rendered.

  Where to call it:
  After specialized rendering doesn't handle the field.
  ==================================================================================*/
  bool render_generic_field(
    const std::string& display_name,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY)
  );

  /*=================================================================================
  Function Brief:
  Renders a field using specialized or generic rendering.

  Parameters:
  display_name - The name to display for the field in the UI
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input
  entity       - Optional entity for context-dependent rendering

  Where to call it:
  When rendering any component field in editors.
  ==================================================================================*/
  void render_field(
    const std::string& display_name,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY)
  );

} // namespace ImGuiWidgets
