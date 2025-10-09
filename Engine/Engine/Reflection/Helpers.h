#pragma once

#include <type_traits>
#include <iostream>
#include <string>
#include "Reflect.h"

template <typename T>
const char* get_class_name()
{
  if constexpr (has_adaptor_v<T>)
  {
    return reflection_adaptor<T>::class_name;
  }
  else if constexpr (has_reflection_v<T>)
  {
    return reflection_info<T>::class_name;
  }
  else
  {
    return "Unknown";
  }
}

template <typename T>
auto get_class_fields()
{
  if constexpr (has_adaptor_v<T>)
  {
    using Adaptor = typename reflection_adaptor<T>::view_type;
    return reflection_info<Adaptor>::fields;
  }
  else if constexpr (has_reflection_v<T>)
  {
    return reflection_info<T>::fields;
  }
  else
  {
    return std::make_tuple();
  }
}

template <typename T, typename Callback>
void iterate_fields(const T& obj, Callback&& callback, int depth = 0)
{
  if constexpr (has_adaptor_v<T>)
  {
    auto view = reflection_adaptor<T>::serialize(obj);
    auto fields = get_class_fields<T>();
    std::apply([&](const auto&... pairs) {
      (callback(pairs.first, view.*pairs.second, depth), ...);
      }, fields);
  }
  else if constexpr (has_reflection_v<T>)
  {
    auto fields = get_class_fields<T>();
    std::apply([&](const auto&... pairs) {
      ([&]() {
        using FieldType = std::remove_cvref_t<decltype(obj.*pairs.second)>;
        if constexpr (has_reflection_v<FieldType> || has_adaptor_v<FieldType>) {
          callback(pairs.first, obj.*pairs.second, depth);
          iterate_fields(obj.*pairs.second, callback, depth + 1);
        }
        else {
          callback(pairs.first, obj.*pairs.second, depth);
        }
        }(), ...);
      }, fields);
  }
}

template <typename T>
void print_struct_fields(const T& obj, int indent = 0)
{
  std::string indent_str(indent, ' ');

  std::cout << indent_str << "Type: [" << get_class_name<T>() << "]";
  if constexpr (has_adaptor_v<T>) {
    std::cout << " (via adaptor)";
  }
  std::cout << ":\n";

  iterate_fields(obj, [indent](const char* name, const auto& value, int depth) {
    using ValueType = std::remove_cvref_t<decltype(value)>;
    std::string field_indent(indent + 2 + depth * 2, ' ');
    std::cout << field_indent << "- " << name << ": ";

    if constexpr (has_reflection_v<ValueType> || has_adaptor_v<ValueType>) {
      std::cout << "[" << get_class_name<ValueType>() << "]";
      if constexpr (has_adaptor_v<ValueType>) {
        std::cout << " (via adaptor)";
      }
      std::cout << "\n";
    }
    else {
      std::cout << value << "\n";
    }
    });
}
