#include "SpriteDropper.h"
#include "core.h"



//my <3 sprite dropper allows you to drag and drop the images onto a space
void SpriteDropper(std::vector<int>& loadedImageIds, std::vector<DroppedImage>& droppedImages) {
    // --- Sprite Dropper ---
    ImGui::Begin("Sprite Dropper");
    ImGui::Text("Drop Images:");

    for (int imageId : loadedImageIds) {
        ImGui::PushID(imageId);

        // Push a style color change to make the button background transparent
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

        // Create the transparent button. Its area still captures clicks and drag events.
        ImGui::Button("##invisible_button", ImVec2(50, 50));

        // Pop the style colors to revert the change for other buttons
        ImGui::PopStyleColor(3);

        // Check if the button is being dragged.
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            // Set the payload with the image ID
            ImGui::SetDragDropPayload("SPRITEDROP_ID", &imageId, sizeof(int));

            // Draw the drag preview
            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(imageId)), ImVec2(50, 50));
            ImGui::Text("Dragging Image...");
            ImGui::EndDragDropSource();
        }

        // Placing a button on top of the image
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(static_cast<intptr_t>(imageId)), min, max, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));

        ImGui::PopID(); // Remove the unique ID from the stack
        ImGui::SameLine(); // Places images on a same line
    }
    ImGui::End();

}