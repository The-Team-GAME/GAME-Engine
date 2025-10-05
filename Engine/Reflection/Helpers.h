#pragma once

template <typename T>
void print_struct_fields(const T& obj)
{
  auto [class_name, fields_tuple] = reflection_fields<T>();

  std::cout << "Type: " << class_name << " has fields:\n";
  std::apply([&obj](const auto &...pairs)
    {
      ((std::cout << "- " << pairs.first << ": " << (obj.*pairs.second) << "\n"), ...);
    },
    fields_tuple
  );
}