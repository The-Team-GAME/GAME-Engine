#include "ComponentSelector.h"
#include "ObjectFactory.h"
#include "imgui.h"

namespace EditorUI
{
  void ComponentSelector::DrawAddComponentButtons(ECS::EntityID entity)
  {
    if (ImGui::Button("Add Component"))
    {
      ImGui::OpenPopup("AddComponentPopup");
    }
     

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
      // Example: list of attachable components
      if (ImGui::MenuItem("Position"))
      {
        Engine::WORLD.add_component<Components::Position>(entity, 0.0f, 0.0f);
      }
      if (ImGui::MenuItem("Velocity"))
      {
        //Engine::WORLD.addComponent<Components::Velocity>(entity, 0.0f, 0.0f);
      }
      if (ImGui::MenuItem("Scale"))
      {
        Engine::WORLD.add_component<Components::Scale>(entity, 1.0f, 1.0f);
      }
      if (ImGui::MenuItem("Tag<GameObjectState>"))
      {
        Engine::WORLD.add_component<Components::Tag<"GameObjectState">>(entity);
      }

      ImGui::EndPopup();

    }
  }
}


