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
#include "../../ECS/Prefab.h"

namespace ImGuiWidgets {

  /*=================================================================================
  Function Brief:
  Parses a component name with namespaces (e.g., "Namespace1::Namespace2::Component")
  and renders it as nested tree nodes. Returns true if the innermost node is open.

  Parameters:
  component_name - The full component name with namespaces
  content_func   - Lambda function to render the component content when innermost node is open

  Returns:
  true if the component tree was rendered successfully.

  Where to call it:
  From component editor panels when rendering component trees.
  ==================================================================================*/
  bool render_nested_component_tree(
    const std::string& component_name,
    const std::function<void()>& content_func
  );

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
  Renders a 2D transform editor for Components::Transform2D fields.

  Parameters:
  field_value - Reference to the GenericValue containing font path and size data
  modified    - [out] Set to true if the font was modified by user input

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for Transform2D components.
  ==================================================================================*/
  bool render_transform2d_field(ReflectionLibrary::GenericValue& field_value, bool& modified);

  /*=================================================================================
  Function Brief:
  Renders a UI anchor editor with a 3x3 preset grid and pixel offset fields
  for Components::UI fields.

  Parameters:
  field_value - Reference to the GenericValue containing anchor and offset data
  modified    - [out] Set to true if the values were modified by user input

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for UI components.
  ==================================================================================*/
  bool render_ui_anchor_field(ReflectionLibrary::GenericValue& field_value, bool& modified, Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY));

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
  Renders an animation editor widget for Components::Animation fields.
  Allows manual editing of frame durations, total duration, loop settings, etc.

  Parameters:
  field_value - Reference to the GenericValue containing animation data
  modified    - [out] Set to true if the animation was modified by user input

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for animation components.
  ==================================================================================*/
  bool render_animation_field(ReflectionLibrary::GenericValue& field_value, bool& modified, Engine::ECS::Entity entity);

  /*=================================================================================
  Function Brief:
  Renders a state machine editor widget for Components::StateMachine fields.
  Displays all states with their textures and allows modifying their animations.
  Includes buttons to open the Frame Animation Editor for each state.

  Parameters:
  field_value - Reference to the GenericValue containing state machine data
  modified    - [out] Set to true if the state machine was modified by user input
  entity      - Optional entity for opening the Frame Animation Editor

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for state machine components.
  ==================================================================================*/
  bool render_statemachine_field(ReflectionLibrary::GenericValue& field_value, bool& modified, Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY));

  /*=================================================================================
  Function Brief:
  Renders a dropdown for Engine::SerializableFunction fields with all registered
  functions. Automatically determines the correct signature based on field name
  (easing functions use f32(f32,f32,f32,f32), callbacks use void(Entity), others use void()).

  Parameters:
  field_value - Reference to the GenericValue containing SerializableFunction data
  modified    - [out] Set to true if the function was modified by user input
  field_name  - Name of the field to determine signature

  Returns:
  true if the field was handled and rendered, false otherwise.

  Where to call it:
  From specialized field renderers for SerializableFunction fields.
  ==================================================================================*/
  bool render_serializable_function_field(ReflectionLibrary::GenericValue& field_value, bool& modified, const std::string& field_name);

  /*=================================================================================
  Function Brief:
  Renders a button to open the Collider Editor for Components::BoxCollider fields.

  Parameters:
  field_value - Reference to the GenericValue containing box collider data
  modified    - [out] Set to true if the collider was modified by user input
  entity      - Optional entity context
  prefab      - Optional prefab context

  Returns:
  true if the field was handled and rendered, false otherwise.
  ==================================================================================*/
  bool render_box_collider_field(ReflectionLibrary::GenericValue& field_value, bool& modified, Engine::ECS::Entity entity, Engine::ECS::Prefab* prefab = nullptr, const std::string& prefab_path = "");

  /*=================================================================================
  Function Brief:
  Renders specialized UI fields for specific component types (e.g., Color, Texture,
  Font). Returns true if the field was handled and rendered, false otherwise.

  Parameters:
  display_name - The name to display for the field in the UI
  type_hint    - The type name used to identify which specialized renderer to use
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
    const std::string& type_hint,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY),
    Engine::ECS::Prefab* prefab = nullptr,
    const std::string& prefab_path = ""
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
    Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY),
    Engine::ECS::Prefab* prefab = nullptr,
    const std::string& prefab_path = ""
  );

  /*=================================================================================
  Function Brief:
  Renders a field using specialized or generic rendering.

  Parameters:
  display_name - The name to display for the field in the UI
  type_hint    - The type name used to identify which specialized renderer to use
  field_value  - Reference to the GenericValue containing the field data
  modified     - [out] Set to true if the field value was modified by user input
  entity       - Optional entity for context-dependent rendering

  Where to call it:
  When rendering any component field in editors.
  ==================================================================================*/
  void render_field(
    const std::string& display_name,
    const std::string& type_hint,
    ReflectionLibrary::GenericValue& field_value,
    bool& modified,
    Engine::ECS::Entity entity = Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY),
    Engine::ECS::Prefab* prefab = nullptr,
    const std::string& prefab_path = ""
  );

} // namespace ImGuiWidgets

