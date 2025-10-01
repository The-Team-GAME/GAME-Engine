#pragma once
#include "imgui.h"
#include "Core.h"

inline void CameraPanel()
{
  ImGui::Begin("Camera Control");

  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Camera Settings");
  ImGui::Separator();

  // Get current camera values through a static accessor
  static glm::vec2 camera_position = { 0.0f, 0.0f };
  static f32 camera_zoom = 1.0f;
  static f32 camera_rotation = 0.0f;

  // Position Controls
  if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::DragFloat("X", &camera_position.x, 1.0f);
    ImGui::DragFloat("Y", &camera_position.y, 1.0f);
    
    if (ImGui::Button("Reset Position"))
    {
      camera_position = { 0.0f, 0.0f };
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Center on Origin"))
    {
      camera_position = { Graphics::VIRTUAL_WIDTH / 2.0f, Graphics::VIRTUAL_HEIGHT / 2.0f };
    }

    // Quick position presets
    if (ImGui::TreeNode("Quick Presets"))
    {
      if (ImGui::Button("Top-Left"))
        camera_position = { 0.0f, 0.0f };
      
      ImGui::SameLine();
      
      if (ImGui::Button("Top-Right"))
        camera_position = { Graphics::VIRTUAL_WIDTH, 0.0f };
      
      if (ImGui::Button("Bottom-Left"))
        camera_position = { 0.0f, Graphics::VIRTUAL_HEIGHT };
      
      ImGui::SameLine();
      
      if (ImGui::Button("Bottom-Right"))
        camera_position = { Graphics::VIRTUAL_WIDTH, Graphics::VIRTUAL_HEIGHT };

      ImGui::TreePop();
    }

    // Apply position
    Engine::GRAPHICS.set_camera_position(camera_position.x, camera_position.y);
  }

  ImGui::Separator();

  // Zoom Controls
  if (ImGui::CollapsingHeader("Zoom", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::SliderFloat("Zoom Level", &camera_zoom, 0.1f, 5.0f, "%.2f");
    
    ImGui::Text("Shortcuts:");
    
    if (ImGui::Button("Zoom In (+25%)"))
    {
      camera_zoom *= 1.25f;
      if (camera_zoom > 5.0f) camera_zoom = 5.0f;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Zoom Out (-25%)"))
    {
      camera_zoom *= 0.75f;
      if (camera_zoom < 0.1f) camera_zoom = 0.1f;
    }

    if (ImGui::Button("Reset Zoom (1.0x)"))
    {
      camera_zoom = 1.0f;
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Fit Screen"))
    {
      camera_zoom = 1.0f;
    }

    // Quick zoom presets
    if (ImGui::TreeNode("Zoom Presets"))
    {
      if (ImGui::Button("0.25x")) camera_zoom = 0.25f;
      ImGui::SameLine();
      if (ImGui::Button("0.5x")) camera_zoom = 0.5f;
      ImGui::SameLine();
      if (ImGui::Button("1.0x")) camera_zoom = 1.0f;
      ImGui::SameLine();
      if (ImGui::Button("2.0x")) camera_zoom = 2.0f;
      ImGui::SameLine();
      if (ImGui::Button("4.0x")) camera_zoom = 4.0f;

      ImGui::TreePop();
    }

    // Apply zoom
    Engine::GRAPHICS.set_camera_zoom(camera_zoom);
  }

  ImGui::Separator();

  // Rotation Controls
  if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::SliderFloat("Angle (degrees)", &camera_rotation, 0.0f, 360.0f, "%.1f°");
    
    // Wrap rotation to 0-360 range
    if (camera_rotation < 0.0f) camera_rotation += 360.0f;
    if (camera_rotation >= 360.0f) camera_rotation -= 360.0f;

    if (ImGui::Button("Rotate +15°"))
    {
      camera_rotation += 15.0f;
      if (camera_rotation >= 360.0f) camera_rotation -= 360.0f;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Rotate -15°"))
    {
      camera_rotation -= 15.0f;
      if (camera_rotation < 0.0f) camera_rotation += 360.0f;
    }

    if (ImGui::Button("Rotate +90°"))
    {
      camera_rotation += 90.0f;
      if (camera_rotation >= 360.0f) camera_rotation -= 360.0f;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Rotate -90°"))
    {
      camera_rotation -= 90.0f;
      if (camera_rotation < 0.0f) camera_rotation += 360.0f;
    }

    if (ImGui::Button("Reset Rotation (0°)"))
    {
      camera_rotation = 0.0f;
    }

    // Quick rotation presets
    if (ImGui::TreeNode("Rotation Presets"))
    {
      if (ImGui::Button("0°")) camera_rotation = 0.0f;
      ImGui::SameLine();
      if (ImGui::Button("45°")) camera_rotation = 45.0f;
      ImGui::SameLine();
      if (ImGui::Button("90°")) camera_rotation = 90.0f;
      ImGui::SameLine();
      if (ImGui::Button("180°")) camera_rotation = 180.0f;
      ImGui::SameLine();
      if (ImGui::Button("270°")) camera_rotation = 270.0f;

      ImGui::TreePop();
    }

    // Apply rotation
    Engine::GRAPHICS.set_camera_rotation(camera_rotation);
  }

  ImGui::Separator();

  // Combined Reset
  if (ImGui::Button("Reset All Camera Settings", ImVec2(-1, 0)))
  {
    camera_position = { 0.0f, 0.0f };
    camera_zoom = 1.0f;
    camera_rotation = 0.0f;
    
    Engine::GRAPHICS.set_camera_position(camera_position.x, camera_position.y);
    Engine::GRAPHICS.set_camera_zoom(camera_zoom);
    Engine::GRAPHICS.set_camera_rotation(camera_rotation);
  }

  ImGui::Separator();

  // Display info
  if (ImGui::CollapsingHeader("Info"))
  {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Virtual Resolution:");
    ImGui::Text("  %.0f x %.0f", Graphics::VIRTUAL_WIDTH, Graphics::VIRTUAL_HEIGHT);
    
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current Camera:");
    ImGui::Text("  Position: (%.1f, %.1f)", camera_position.x, camera_position.y);
    ImGui::Text("  Zoom: %.2fx", camera_zoom);
    ImGui::Text("  Rotation: %.1f°", camera_rotation);

    ImGui::Separator();
    
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Tips:");
    ImGui::TextWrapped("• Use position controls to pan the camera");
    ImGui::TextWrapped("• Zoom > 1.0 zooms in, < 1.0 zooms out");
    ImGui::TextWrapped("• Rotation rotates the entire view");
  }

  ImGui::End();
}