//-----------------------------------------------------------------------------------
// file:    SpriteDropper.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file implements the SpriteDropper UI function. It displays all loaded
//          image assets as drag sources and prepares the texture ID payload for
//          drag-and-drop operations, allowing assets to be placed onto other
//          target windows, such as the Timeline.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "SpriteDropper.h"
#include "core.h"
#include "imgui.h"

/*=================================================================================
    Function Brief:
    Renders the "Sprite Dropper" ImGui window, which lists all loaded images.
    It sets up each image as a drag source, packaging the texture ID as a payload
    for other components (like the Timeline) to receive.

    Where to call it:
    Must be called once per frame inside the game state's main Draw() function
    (e.g., SpriteState::Draw()).
    ==================================================================================*/
void SpriteDropper(std::vector<int>& loadedImageIds) {

  // --- Sprite Dropper ---
  ImGui::Begin("Sprite Dropper");
  // NOTE: Changed text to reflect its primary role as a source, not a drop zone
  ImGui::Text("Drag Images to Timeline:");

  for (s32 image_id : loadedImageIds) {
    ImGui::PushID(image_id);

    // Push a style color change to make the button background transparent
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f)); // Changed hover color for visibility

    // Create the transparent button. Its area still captures clicks and drag events.
    // This is the active area for initiating the drag.
    ImGui::Button("##invisible_button", ImVec2(50, 50));

    // Pop the style colors to revert the change for other buttons
    ImGui::PopStyleColor(3);

    // Check if the button is being dragged (Drag Source)
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {

      // Set the payload with the image ID
      // The string "SPRITEDROP_ID" links this source to the Timeline's drop target.
      ImGui::SetDragDropPayload("SPRITEDROP_ID", &image_id, sizeof(int));

      // Draw the drag preview that follows the mouse
      ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(image_id)), ImVec2(50, 50));
      ImGui::Text("Dragging Image...");

      ImGui::EndDragDropSource();
    }

    // Placing the actual texture on top of the button area to make it visible
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    ImGui::GetWindowDrawList()->AddImage(
      reinterpret_cast<void*>(static_cast<intptr_t>(image_id)),
      min, max,
      ImVec2(0, 0), ImVec2(1, 1),
      ImGui::GetColorU32(ImVec4(1, 1, 1, 1))
    );

    ImGui::PopID(); // Remove the unique ID from the stack
    ImGui::SameLine(); // Places images on a same line

  }
  ImGui::End();
}