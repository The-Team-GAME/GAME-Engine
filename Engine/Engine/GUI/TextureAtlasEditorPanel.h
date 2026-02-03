//-----------------------------------------------------------------------------------
// file:    TextureAtlasEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ImGui panel for editing TextureAtlas component parameters.
//          Allows users to configure rows, columns, tile dimensions, and
//          total subtextures for an entity's texture atlas. Provides live
//          preview of the atlas grid configuration.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Engine::ECS {
    class Entity;
}

namespace TextureAtlasEditor {

    /*=================================================================================
    Function Brief:
    Opens the Texture Atlas Editor panel for editing an entity's TextureAtlas component.

    Parameters:
    entity - The entity with Texture and TextureAtlas components

    Where to call it:
    Called from the Entity Editor when user wants to configure a TextureAtlas.
    ==================================================================================*/
    void Open(Engine::ECS::Entity entity);

    /*=================================================================================
    Function Brief:
    Opens the Texture Atlas Editor panel for editing a specific state's texture atlas
    within a StateMachine component.

    Parameters:
    entity - The entity with a StateMachine component
    state_name - The name of the state whose texture atlas should be edited

    Where to call it:
    Called from the StateMachine inspector when user wants to configure a state's
    texture atlas grid.
    ==================================================================================*/
    void OpenForState(Engine::ECS::Entity entity, const std::string& state_name);

    /*=================================================================================
    Function Brief:
    Closes the Texture Atlas Editor panel and clears any selection state.

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
Renders the Texture Atlas Editor ImGui panel. Displays texture preview with
atlas grid overlay, and provides controls to modify rows, columns, tile size,
and total subtexture count.

Where to call it:
Called during the ImGui phase of the main game loop via panel registration.
==================================================================================*/
void TextureAtlasEditorPanel();
