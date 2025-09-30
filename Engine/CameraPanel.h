#pragma once
#include "imgui.h"
#include "Core.h"

inline void CameraPanel()
{
  ImGui::Begin("Camera Control");

  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Camera Settings");
  ImGui::Separator();

  // Get current camera values through a static accessor
  static glm::vec2 cameraPosition = { 0.0f, 0.0f };
  static float cameraZoom = 1.0f;
  static float cameraRotation = 0.0f;

  // Position Controls
  if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::DragFloat("X", &cameraPosition.x, 1.0f);
    ImGui::DragFloat("Y", &cameraPosition.y, 1.0f);
    
    if (ImGui::Button("Reset Position"))
    {
      cameraPosition = { 0.0f, 0.0f };
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Center on Origin"))
    {
      cameraPosition = { Graphics::VIRTUAL_WIDTH / 2.0f, Graphics::VIRTUAL_HEIGHT / 2.0f };
    }

    // Quick position presets
    if (ImGui::TreeNode("Quick Presets"))
    {
      if (ImGui::Button("Top-Left"))
        cameraPosition = { 0.0f, 0.0f };
      
      ImGui::SameLine();
      
      if (ImGui::Button("Top-Right"))
        cameraPosition = { Graphics::VIRTUAL_WIDTH, 0.0f };
      
      if (ImGui::Button("Bottom-Left"))
        cameraPosition = { 0.0f, Graphics::VIRTUAL_HEIGHT };
      
      ImGui::SameLine();
      
      if (ImGui::Button("Bottom-Right"))
        cameraPosition = { Graphics::VIRTUAL_WIDTH, Graphics::VIRTUAL_HEIGHT };

      ImGui::TreePop();
    }

    // Apply position
    Engine::GRAPHICS.setCameraPosition(cameraPosition.x, cameraPosition.y);
  }

  ImGui::Separator();

  // Zoom Controls
  if (ImGui::CollapsingHeader("Zoom", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::SliderFloat("Zoom Level", &cameraZoom, 0.1f, 5.0f, "%.2f");
    
    ImGui::Text("Shortcuts:");
    
    if (ImGui::Button("Zoom In (+25%)"))
    {
      cameraZoom *= 1.25f;
      if (cameraZoom > 5.0f) cameraZoom = 5.0f;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Zoom Out (-25%)"))
    {
      cameraZoom *= 0.75f;
      if (cameraZoom < 0.1f) cameraZoom = 0.1f;
    }

    if (ImGui::Button("Reset Zoom (1.0x)"))
    {
      cameraZoom = 1.0f;
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Fit Screen"))
    {
      cameraZoom = 1.0f;
    }

    // Quick zoom presets
    if (ImGui::TreeNode("Zoom Presets"))
    {
      if (ImGui::Button("0.25x")) cameraZoom = 0.25f;
      ImGui::SameLine();
      if (ImGui::Button("0.5x")) cameraZoom = 0.5f;
      ImGui::SameLine();
      if (ImGui::Button("1.0x")) cameraZoom = 1.0f;
      ImGui::SameLine();
      if (ImGui::Button("2.0x")) cameraZoom = 2.0f;
      ImGui::SameLine();
      if (ImGui::Button("4.0x")) cameraZoom = 4.0f;

      ImGui::TreePop();
    }

    // Apply zoom
    Engine::GRAPHICS.setCameraZoom(cameraZoom);
  }

  ImGui::Separator();

  // Rotation Controls
  if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::SliderFloat("Angle (degrees)", &cameraRotation, 0.0f, 360.0f, "%.1f°");
    
    // Wrap rotation to 0-360 range
    if (cameraRotation < 0.0f) cameraRotation += 360.0f;
    if (cameraRotation >= 360.0f) cameraRotation -= 360.0f;

    if (ImGui::Button("Rotate +15°"))
    {
      cameraRotation += 15.0f;
      if (cameraRotation >= 360.0f) cameraRotation -= 360.0f;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Rotate -15°"))
    {
      cameraRotation -= 15.0f;
      if (cameraRotation < 0.0f) cameraRotation += 360.0f;
    }

    if (ImGui::Button("Rotate +90°"))
    {
      cameraRotation += 90.0f;
      if (cameraRotation >= 360.0f) cameraRotation -= 360.0f;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Rotate -90°"))
    {
      cameraRotation -= 90.0f;
      if (cameraRotation < 0.0f) cameraRotation += 360.0f;
    }

    if (ImGui::Button("Reset Rotation (0°)"))
    {
      cameraRotation = 0.0f;
    }

    // Quick rotation presets
    if (ImGui::TreeNode("Rotation Presets"))
    {
      if (ImGui::Button("0°")) cameraRotation = 0.0f;
      ImGui::SameLine();
      if (ImGui::Button("45°")) cameraRotation = 45.0f;
      ImGui::SameLine();
      if (ImGui::Button("90°")) cameraRotation = 90.0f;
      ImGui::SameLine();
      if (ImGui::Button("180°")) cameraRotation = 180.0f;
      ImGui::SameLine();
      if (ImGui::Button("270°")) cameraRotation = 270.0f;

      ImGui::TreePop();
    }

    // Apply rotation
    Engine::GRAPHICS.setCameraRotation(cameraRotation);
  }

  ImGui::Separator();

  // Combined Reset
  if (ImGui::Button("Reset All Camera Settings", ImVec2(-1, 0)))
  {
    cameraPosition = { 0.0f, 0.0f };
    cameraZoom = 1.0f;
    cameraRotation = 0.0f;
    
    Engine::GRAPHICS.setCameraPosition(cameraPosition.x, cameraPosition.y);
    Engine::GRAPHICS.setCameraZoom(cameraZoom);
    Engine::GRAPHICS.setCameraRotation(cameraRotation);
  }

  ImGui::Separator();

  // Display info
  if (ImGui::CollapsingHeader("Info"))
  {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Virtual Resolution:");
    ImGui::Text("  %.0f x %.0f", Graphics::VIRTUAL_WIDTH, Graphics::VIRTUAL_HEIGHT);
    
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current Camera:");
    ImGui::Text("  Position: (%.1f, %.1f)", cameraPosition.x, cameraPosition.y);
    ImGui::Text("  Zoom: %.2fx", cameraZoom);
    ImGui::Text("  Rotation: %.1f°", cameraRotation);

    ImGui::Separator();
    
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Tips:");
    ImGui::TextWrapped("• Use position controls to pan the camera");
    ImGui::TextWrapped("• Zoom > 1.0 zooms in, < 1.0 zooms out");
    ImGui::TextWrapped("• Rotation rotates the entire view");
  }

  ImGui::End();
}