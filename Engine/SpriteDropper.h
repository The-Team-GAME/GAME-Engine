//-----------------------------------------------------------------------------------
// file:    SpriteDropper.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file aids in making the functions that help 
//					implement the SpriteDropper UI function. It displays all loaded
//          image assets as drag sources and prepares the texture ID payload for
//          drag-and-drop operations, allowing assets to be placed onto other
//          target windows, such as the Timeline.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.

#pragma once

#include "core.h"
#include <imgui.h>
#include <vector>

struct DroppedImage {
	int textureId;
	ImVec2 position;
	bool isDropped = false;
};

// Function to render the Sprite Dropper UI.
// It takes the list of loaded images to display and a reference to the vector
// where dropped images should be stored.
void SpriteDropper(std::vector<int>& loadedImageIds, std::vector<DroppedImage>& droppedImages);