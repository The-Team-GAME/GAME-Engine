//-----------------------------------------------------------------------------------
// file:    ComponentSelector.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   This file defines the ComponentSelector class, which provides UI
//          functionality for adding and managing components in the editor.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once
#include "Core.h"


namespace EditorUI {
	class ComponentSelector
	{
	public:
		static void DrawAddComponentButtons(ECS::EntityID entityId);
		static void DrawComponentList(ECS::EntityID entityId);
	};
}

