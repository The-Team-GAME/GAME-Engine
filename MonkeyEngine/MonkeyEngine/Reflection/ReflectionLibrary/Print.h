//-----------------------------------------------------------------------------------
// file:    Print.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Debug printing utilities for the reflection library. Provides
//          formatted output of reflectable objects and their fields for
//          debugging and development purposes. Handles various data types
//          including primitives, containers, and nested structures.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <string_view>
#include "../../Logger.h"

namespace ReflectionLibrary
{
  template <typename T>
  static void print_value(std::string_view name, T& value, int nest_level)
  {
    std::string indent(2 + nest_level * 2, ' ');
    using ValueType = std::decay_t<T>;
    if constexpr (is_optional_v<ValueType>)
    {
      // Handle std::optional
      if (!value.has_value())
      {
        Log::info("{}{} = <null>", indent, name);
      }
      else
      {
        auto& inner_value = value.value();
        print_value(name, inner_value, nest_level);
      }
    }
    else if constexpr (is_reflected_enum_v<ValueType>)
    {
      // Handle reflected enums
      Log::info("{}{} = {}", indent, name, enum_to_string(value));
    }
    else if constexpr (std::is_same_v<ValueType, bool>)
    {
      Log::info("{}{} = {}", indent, name, (value ? "true" : "false"));
    }
    else if constexpr (std::is_same_v<ValueType, char> || std::is_same_v<ValueType, unsigned char>)
    {
      Log::info("{}{} = '{}'", indent, name, value);
    }
    else if constexpr (std::is_arithmetic_v<ValueType>)
    {
      // Other arithmetic types (int, float, etc.)
      Log::info("{}{} = {}", indent, name, value);
    }
    else if constexpr (std::is_same_v<ValueType, std::string>)
    {
      Log::info("{}{} = \"{}\"", indent, name, value);
    }
    else if constexpr (is_iterable_v<ValueType>)
    {
      // Iterable containers (arrays, vectors, lists, maps, etc.)
      Log::info("{}{} = [", indent, name);
      int i = 0;
      for (auto& elem : value)
      {
        print_value(std::to_string(i++), elem, nest_level + 1);
      }
      Log::info("{}]", indent);
    }
    else if constexpr (is_pair_v<ValueType>)
    {
      Log::info("{}{} = {{", indent, name);
      print_value("first", value.first, nest_level + 1);
      print_value("second", value.second, nest_level + 1);
      Log::info("{}}}", indent);
    }
    else if constexpr (is_tuple_v<ValueType>)
    {
      Log::info("{}{} = (", indent, name);
      std::apply([&](auto &...args)
        {
          int i = 0;
          ((print_value(std::to_string(i++), args, nest_level + 1)), ...); }, value);
      Log::info("{})", indent);
    }
    else if constexpr (is_reflectable_v<ValueType>)
    {
      // Nested reflectable type - print header and recurse into fields
      Log::info("{}{} ({}):", indent, name, ReflectionInfo<ValueType>::class_name);
      for_each_field(value, [&](std::string_view sub_name, auto& sub_value, int)
        { print_value(sub_name, sub_value, nest_level + 1); }, nest_level + 1);
    }
    else
    {
      // Fallback for other non-reflectable types
      Log::info("{}{} = [non-reflectable type]", indent, name);
    }
  }

  template <typename T>
  static void print(T& obj)
  {
    Log::info("Type: {}", ReflectionInfo<T>::class_name);
    for_each_field(obj, [](std::string_view name, auto& value, int nest_level)
      { print_value(name, value, nest_level); }, 0);
  }
} // namespace ReflectionLibrary
