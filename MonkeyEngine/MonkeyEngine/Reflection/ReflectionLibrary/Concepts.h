//-----------------------------------------------------------------------------------
// file:    Concepts.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Defines C++20 concepts used throughout the reflection library.
//          Provides compile-time type checking for reflectable types, tuples,
//          pairs, and other data structures. Enables template metaprogramming
//          for automatic reflection capabilities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <type_traits>
#include <concepts>
#include <array>
#include <optional>

namespace ReflectionLibrary
{
  // Forward declarations
  template <typename T>
  struct ReflectionInfo;

  template <typename T>
  struct EnumInfo;

  // Check if a type is reflectable
  template <typename T>
  concept is_reflectable_v = requires {
    ReflectionInfo<T>::class_name;
    ReflectionInfo<T>::fields;
    ReflectionInfo<T>::reflect;
    ReflectionInfo<T>::create;
  };

  // Check if a type is a reflected enum
  template <typename T>
  concept is_reflected_enum_v = std::is_enum_v<T> && requires {
    EnumInfo<T>::enum_name;
    EnumInfo<T>::values;
  };

  // Check if a type is a std::tuple
  template <typename T>
  concept is_tuple_v = requires { std::tuple_size<T>::value; };

  // Check if a type is a std::pair
  template <typename T>
  concept is_pair_v = requires { typename T::first_type; typename T::second_type; };

  // Check if a type is iterable (has size, begin, end)
  template <typename T>
  concept is_iterable_v = requires(T value) { value.size(); value.begin(); value.end(); };

  // Check if a type is std::optional
  template <typename T>
  concept is_optional_v = requires(T value) { value.has_value(); value.value(); };

  template <typename T>
  concept has_reflector_v = requires(const T & obj) {
    typename T::Reflector;
    { T::Reflector::reflect(obj) } -> std::same_as<typename T::Reflector>;
    { T::Reflector::create(std::declval<typename T::Reflector>()) } -> std::same_as<T>;
  };

}