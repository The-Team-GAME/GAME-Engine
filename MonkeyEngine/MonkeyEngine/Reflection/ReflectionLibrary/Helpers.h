//-----------------------------------------------------------------------------------
// file:    Helpers.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Utility functions and templates for the reflection library.
//          Provides compile-time field counting, class name retrieval,
//          and iteration over reflected fields. Enables generic operations
//          on reflectable types through template metaprogramming.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string_view>
#include <tuple>

namespace ReflectionLibrary
{
  template <typename T>
  static constexpr std::string_view class_name = ReflectionInfo<T>::class_name;

  template <typename T>
  static constexpr std::string_view get_class_name(const T&) { return class_name<T>; }

  template <typename T>
  static constexpr size_t field_count = std::tuple_size_v<decltype(ReflectionInfo<T>::fields)>;

  template <typename T>
  static constexpr size_t get_field_count(const T&) { return field_count<T>; }

  template <typename T>
  static void for_each_field(T& obj, auto&& func, int nest_level = 0)
  {
    auto reflector = ReflectionInfo<T>::reflect(obj);
    std::apply([&](auto &&...field)
      { (([&]()
        {
          auto& val = reflector.*(field.second);
          func(field.first, val, nest_level); }()),
        ...); }, ReflectionInfo<T>::fields);
    obj = ReflectionInfo<T>::create(reflector);
  }

  template <typename T, typename U>
  static void set_field_value(T& obj, std::string_view field_name, U&& value)
  {
    const auto setter_func = [&](std::string_view name, auto& val)
      {
        if (name == field_name)
        {
          using FieldType = std::decay_t<decltype(val)>;
          if constexpr (std::is_assignable_v<FieldType&, decltype(value)>)
          {
            val = std::forward<U>(value);
          }
        }
      };

    auto reflector = ReflectionInfo<T>::reflect(obj);
    std::apply([&](auto &&...fields)
      { ((setter_func(fields.first, reflector.*(fields.second))),
        ...); }, ReflectionInfo<T>::fields);
    obj = ReflectionInfo<T>::create(reflector);
  }

  // Enum helper functions
  template <typename T>
    requires is_reflected_enum_v<T>
  static constexpr std::string_view enum_to_string(T value)
  {
    std::string_view result;
    std::apply([&](auto &&...pairs)
      {
        ((pairs.first == value ? (result = pairs.second, true) : false) || ...);
      }, EnumInfo<T>::values);
    return result;
  }

  template <typename T>
    requires is_reflected_enum_v<T>
  static constexpr std::optional<T> string_to_enum(std::string_view str)
  {
    std::optional<T> result;
    std::apply([&](auto &&...pairs)
      {
        ((pairs.second == str ? (result = pairs.first, true) : false) || ...);
      }, EnumInfo<T>::values);
    return result;
  }

  template <typename T>
    requires is_reflected_enum_v<T>
  static constexpr size_t enum_value_count = std::tuple_size_v<decltype(EnumInfo<T>::values)>;

  template <typename T>
    requires is_reflected_enum_v<T>
  static constexpr std::string_view get_enum_name()
  {
    return EnumInfo<T>::enum_name;
  }

} // namespace ReflectionLibrary