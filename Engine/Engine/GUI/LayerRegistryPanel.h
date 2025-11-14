//-----------------------------------------------------------------------------------
// file:    LayerRegistryPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares the LayerRegistryPanel ImGui interface for configuring
//          the layer collision matrix. Allows users to enable/disable collisions
//          between different layers in a visual grid interface.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/*=================================================================================
Function Brief:
Renders the Layer Registry panel for configuring layer collision relationships.
Displays a grid showing which layers collide with which other layers and allows
interactive editing of the collision matrix.

Where to call it:
Called from the main ImGui render loop, typically alongside other editor panels.
==================================================================================*/
void LayerRegistryPanel();
