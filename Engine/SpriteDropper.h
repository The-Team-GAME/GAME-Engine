#pragma once

#include "core.h"
#include <imgui.h>
#include <vector>

struct DroppedImage {
	int textureId;
	ImVec2 position;
};

// Function to render the Sprite Dropper UI.
// It takes the list of loaded images to display and a reference to the vector
// where dropped images should be stored.
void SpriteDropper(std::vector<int>& loadedImageIds, std::vector<DroppedImage>& droppedImages);