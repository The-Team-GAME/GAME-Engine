#include "Timeline.h"
#include "imgui.h"

Timeline::Timeline()
  : currentFrame(0),
  numFrames(100),             // start with 100 frames (expandable later)
  zoom(1.0f),                 //how zoomed in is the timeline onto the frames
  cellSize(ImVec2(60, 40))    // size per frame box

{
  // Start with 2 layers
  for ( ; currentFrame < 2; ++currentFrame)
    layers.push_back(currentFrame);
}

// --Timeline--
void Timeline::Draw() {
  if (ImGui::Begin("Timeline"))
  {
    ImVec2 timelineSize(cellSize.x * numFrames, cellSize.y * (int)layers.size());

    // --- Control buttons ---
    if (ImGui::Button("Add")) { //adds layer
      layers.push_back((int)layers.size());
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove") && !layers.empty() && layers.size() != 1) { //removes layer
      layers.pop_back();
    }
    // Add the zoom buttons
    ImGui::SameLine();
    if (ImGui::Button("+")) { //zooms in timeline
      zoom += 0.5f;
      // You can clamp the max zoom level if needed
      if (zoom > 5.0f) zoom = 5.0f;
    }
    
    //Add the zoom slider
    ImGui::SameLine();


    ImGui::SameLine();
    if (ImGui::Button("-")) { //zooms out timeline
      zoom -= 0.5f;
      // Clamp the min zoom level
      if (zoom < 0.2f) zoom = 0.2f;
    }


    // --- Timeline region ---
    ImGui::BeginChild("TimelineRegion", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();


    ImVec2 currentCellSize = ImVec2(cellSize.x * zoom, cellSize.y);

    //Draw grid & cells
    for (int layer = 0; layer < (int)layers.size(); ++layer) {
      for (int frame = 0; frame < numFrames; ++frame) {
        ImVec2 cellMin = ImVec2(p.x + frame * currentCellSize.x, p.y + layer * currentCellSize.y);
        ImVec2 cellMax = ImVec2(cellMin.x + currentCellSize.x, cellMin.y + currentCellSize.y);

        // Draw background rect
        draw_list->AddRect(cellMin, cellMax, IM_COL32(100, 100, 100, 255));

        // Make cell clickable
        ImGui::SetCursorScreenPos(cellMin);
        if (ImGui::InvisibleButton(
          (std::string("cell_") + std::to_string(layer) + "_" + std::to_string(frame)).c_str(),
          currentCellSize))
        {
          currentFrame = frame;
        }
      }
    }

    // Draw playhead
    ImVec2 headMin = ImVec2(p.x + currentFrame * cellSize.x, p.y);
    ImVec2 headMax = ImVec2(headMin.x + 2, p.y + timelineSize.y);
    draw_list->AddRectFilled(headMin, headMax, IM_COL32(255, 0, 0, 255));

    ImGui::EndChild();
  }
  ImGui::End();
}
