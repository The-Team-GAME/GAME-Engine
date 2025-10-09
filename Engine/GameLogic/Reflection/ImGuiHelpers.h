#include "../../Engine/MonkeyEngine.h"


// Display struct fields in ImGui
template <typename T>
void display_struct_in_imgui(T& obj)
{
  /*auto [class_name, fields_tuple] = get_class_fields<T>();

  std::apply([&obj](const auto &...pairs)
    {
      ((
        [&]()
        {
          using FieldType = std::remove_cvref_t<decltype(obj.*pairs.second)>;
          display_imgui_field<FieldType>(pairs.first, obj.*pairs.second);
        }()
          ), ...);
    },
    fields_tuple
  );*/
}

// Helper to display value in ImGui
template<typename T>
void display_imgui_field(const char* label, T& data)
{
  //if constexpr (has_adaptor_v<T>)
  //{
  //  // Handle types with adaptors
  //  using Adaptor = reflection_adaptor<T>;
  //  auto view = Adaptor::serialize(data);

  //  if (ImGui::TreeNode(label))
  //  {
  //    display_struct_in_imgui(view);
  //    // Note: This won't update the original data since view is a copy
  //    // To make this work, you'd need a more sophisticated two-way binding
  //    ImGui::TreePop();
  //  }
  //}
  //else if constexpr (std::is_same_v<T, std::string>)
  //{
  //  char buffer[256];
  //  strncpy_s(buffer, data.c_str(), sizeof(buffer) - 1);
  //  buffer[sizeof(buffer) - 1] = '\0';
  //  if (ImGui::InputText(label, buffer, sizeof(buffer)))
  //  {
  //    data = buffer;
  //  }
  //}
  //else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int32_t>)
  //{
  //  ImGui::DragInt(label, &data);
  //}
  //else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, uint32_t>)
  //{
  //  int temp = static_cast<int>(data);
  //  if (ImGui::DragInt(label, &temp, 1.0f, 0))
  //  {
  //    data = static_cast<unsigned int>(temp);
  //  }
  //}
  //else if constexpr (std::is_same_v<T, float>)
  //{
  //  ImGui::DragFloat(label, &data);
  //}
  //else if constexpr (std::is_same_v<T, double>)
  //{
  //  float temp = static_cast<float>(data);
  //  if (ImGui::DragFloat(label, &temp))
  //  {
  //    data = static_cast<double>(temp);
  //  }
  //}
  //else if constexpr (std::is_same_v<T, bool>)
  //{
  //  ImGui::Checkbox(label, &data);
  //}
  //else if constexpr (has_reflection_v<T>)
  //{
  //  // Handle nested reflected structs
  //  if (ImGui::TreeNode(label))
  //  {
  //    display_struct_in_imgui(data);
  //    ImGui::TreePop();
  //  }
  //}
}
