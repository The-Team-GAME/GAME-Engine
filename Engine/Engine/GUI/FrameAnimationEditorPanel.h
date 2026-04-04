//-----------------------------------------------------------------------------------
// file:    FrameAnimationEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ImGui panel for editing animation frames within state machine states.
//          Allows users to select specific frame indices from a texture atlas and
//          set custom durations for each frame. Supports visual frame selection,
//          reordering, and preview of the resulting animation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Engine::ECS {
  class Entity;
}

namespace FrameAnimationEditor {

  /*=================================================================================
  Function Brief:
  Opens the Frame Animation Editor panel for editing a specific state's animation.

  Parameters:
  entity - The entity with a StateMachine component
  state_name - The name of the state whose animation should be edited

  Where to call it:
  Called from the Entity Editor or State Machine inspector when user wants to edit
  a state's animation frames.
  ==================================================================================*/
  void Open(Engine::ECS::Entity entity, const std::string& state_name);

  /*=================================================================================
  Function Brief:
  Opens the Frame Animation Editor panel for editing an entity's standalone Animation
  component directly. The entity must have Animation, Texture, and TextureAtlas components.

  Parameters:
  entity - The entity with Animation, Texture, and TextureAtlas components

  Where to call it:
  Called from the Entity Editor when user wants to edit an Animation component's frames.
  ==================================================================================*/
  void OpenForAnimation(Engine::ECS::Entity entity);

  /*=================================================================================
  Function Brief:
  Closes the Frame Animation Editor panel and clears any selection state.

  Where to call it:
  Called when user clicks close button or switches to a different entity.
  ==================================================================================*/
  void Close();

  /*=================================================================================
  Function Brief:
  Checks if the editor is currently open.

  Returns:
  True if the editor panel is open, false otherwise.
  ==================================================================================*/
  bool IsOpen();

}

/*=================================================================================
Function Brief:
Renders the Frame Animation Editor ImGui panel. Displays texture atlas with
selectable frames, frame list editor, duration controls, and animation preview.

Where to call it:
Called during the ImGui phase of the main game loop via panel registration.
==================================================================================*/
void FrameAnimationEditorPanel();

