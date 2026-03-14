//-----------------------------------------------------------------------------------
// file:    LuaBindings.h
// project: Monkey Stole My GPA
//
// brief:   Header-only template utilities for automatically binding reflected C++
//          types to Sol2/Lua. Provides:
//            - bind_reflected_type<T>   (REFLECT_FIELDS types)
//            - bind_reflected_custom<T> (REFLECT_CUSTOM types via Reflector proxy)
//            - bind_reflected_enum<E>   (REFLECT_ENUM types)
//            - Recursive field-type discovery & binding
//            - bound_types tracking set to avoid double-binding
//
// Copyright (c) 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <sol/sol.hpp>
#include <string>
#include <string_view>
#include <unordered_set>
#include <typeindex>
#include "Reflection/ReflectionLibrary.h"

namespace Engine::LuaBindings {

  // ============================================================================
  // Tracking set — prevents binding the same C++ type to Lua twice
  // ============================================================================

  inline std::unordered_set<std::type_index>& bound_types() {
    static std::unordered_set<std::type_index> s;
    return s;
  }

  template <typename T>
  bool is_bound() {
    return bound_types().count(std::type_index(typeid(T))) > 0;
  }

  template <typename T>
  void mark_bound() {
    bound_types().insert(std::type_index(typeid(T)));
  }

  // ============================================================================
  // short_name — strips namespace prefix from a fully-qualified name
  //   "Components::Transform2D" -> "Transform2D"
  //   "Math::Vec2D"             -> "Vec2D"
  // ============================================================================

  inline std::string short_name(std::string_view full) {
    auto pos = full.rfind(':');
    if (pos != std::string_view::npos) return std::string(full.substr(pos + 1));
    return std::string(full);
  }

  // ============================================================================
  // Type traits for extracting member-pointer value types
  // ============================================================================

  // member_value_t<&Foo::bar> -> type of bar
  template <typename T>
  struct member_value;

  template <typename C, typename V>
  struct member_value<V C::*> {
    using type = V;
  };

  template <typename T>
  using member_value_t = typename member_value<T>::type;

  // unwrap_t strips const/volatile/reference
  template <typename T>
  using unwrap_t = std::remove_cvref_t<T>;

  // ============================================================================
  // is_lua_safe_field — detects types that Sol2 can handle without issues
  //
  // Sol2 auto-binds std::vector/std::array as containers, which requires
  // operator== on element types. Types like std::vector<std::array<Vec2D,4>>
  // fail because Vec2D has no operator==. We skip these fields entirely.
  // ============================================================================

  // Detect std::vector
  template <typename T> struct is_std_vector : std::false_type {};
  template <typename T, typename A> struct is_std_vector<std::vector<T, A>> : std::true_type {};

  // Detect std::array
  template <typename T> struct is_std_array : std::false_type {};
  template <typename T, size_t N> struct is_std_array<std::array<T, N>> : std::true_type {};

  // Detect std::unordered_map
  template <typename T> struct is_std_unordered_map : std::false_type {};
  template <typename K, typename V, typename... Rest>
  struct is_std_unordered_map<std::unordered_map<K, V, Rest...>> : std::true_type {};

  // Detect std::map
  template <typename T> struct is_std_map : std::false_type {};
  template <typename K, typename V, typename... Rest>
  struct is_std_map<std::map<K, V, Rest...>> : std::true_type {};

  // Detect std::optional
  template <typename T> struct is_std_optional : std::false_type {};
  template <typename T> struct is_std_optional<std::optional<T>> : std::true_type {};

  // A field is "Lua-safe" if it's NOT a container/optional type.
  // Primitive types (int, float, bool, string), reflectable types, enums,
  // and custom-reflector types are all safe. Containers would need
  // operator== on their elements and cause compilation failures.
  template <typename T>
  constexpr bool is_lua_safe_field_v =
    !is_std_vector<T>::value &&
    !is_std_array<T>::value &&
    !is_std_unordered_map<T>::value &&
    !is_std_map<T>::value &&
    !is_std_optional<T>::value;

  // ============================================================================
  // Forward declarations
  // ============================================================================

  template <typename T>
  void bind_reflected_type(sol::state& lua);

  template <typename T>
  void bind_reflected_custom(sol::state& lua);

  template <typename T>
    requires ReflectionLibrary::is_reflected_enum_v<T>
  void bind_reflected_enum(sol::state& lua);

  // ============================================================================
  // bind_field_type_if_needed — recursively binds field types before the parent
  // ============================================================================

  // Primary template: non-reflectable, non-enum → skip
  template <typename FieldType>
  void bind_field_type_if_needed([[maybe_unused]] sol::state& lua) {
    // Leaf type (int, float, string, bool, etc.) — nothing to bind
  }

  // Overload for reflectable types (REFLECT_FIELDS)
  template <typename FieldType>
    requires (ReflectionLibrary::is_reflectable_v<FieldType> &&
              !ReflectionLibrary::has_reflector_v<FieldType> &&
              !ReflectionLibrary::is_reflected_enum_v<FieldType>)
  void bind_field_type_if_needed(sol::state& lua) {
    if (!is_bound<FieldType>()) {
      bind_reflected_type<FieldType>(lua);
    }
  }

  // Overload for custom-reflector types (REFLECT_CUSTOM)
  template <typename FieldType>
    requires (ReflectionLibrary::has_reflector_v<FieldType> &&
              !ReflectionLibrary::is_reflected_enum_v<FieldType>)
  void bind_field_type_if_needed(sol::state& lua) {
    if (!is_bound<FieldType>()) {
      bind_reflected_custom<FieldType>(lua);
    }
  }

  // Overload for reflected enums
  template <typename FieldType>
    requires ReflectionLibrary::is_reflected_enum_v<FieldType>
  void bind_field_type_if_needed(sol::state& lua) {
    if (!is_bound<FieldType>()) {
      bind_reflected_enum<FieldType>(lua);
    }
  }

  // ============================================================================
  // bind_reflected_type<T> — auto-binds a REFLECT_FIELDS type
  //
  // For each field in ReflectionInfo<T>::fields:
  //   1. Recursively bind the field's type if it's reflectable
  //   2. Register the field as a direct pointer-to-member with Sol2
  //
  // The type is registered under its short name (e.g. "Transform2D").
  // ============================================================================

  template <typename T>
  void bind_reflected_type(sol::state& lua) {
    if (is_bound<T>()) return;
    mark_bound<T>();

    std::string name = short_name(ReflectionLibrary::ReflectionInfo<T>::class_name);

    // Phase 1: recursively bind all Lua-safe field types first
    std::apply([&](auto&&... field_pairs) {
      (([&]() {
        using FieldT = unwrap_t<member_value_t<decltype(field_pairs.second)>>;
        if constexpr (is_lua_safe_field_v<FieldT>) {
          bind_field_type_if_needed<FieldT>(lua);
        }
      }()),
       ...);
    }, ReflectionLibrary::ReflectionInfo<T>::fields);

    // Phase 2: create the usertype, only exposing Lua-safe fields
    auto ut = lua.new_usertype<T>(name, sol::constructors<T()>());

    std::apply([&](auto&&... field_pairs) {
      (([&]() {
        using FieldT = unwrap_t<member_value_t<decltype(field_pairs.second)>>;
        if constexpr (is_lua_safe_field_v<FieldT>) {
          ut[field_pairs.first] = field_pairs.second;
        }
      }()),
       ...);
    }, ReflectionLibrary::ReflectionInfo<T>::fields);
  }

  // ============================================================================
  // bind_reflected_custom<T> — auto-binds a REFLECT_CUSTOM type
  //
  // These types use a Reflector proxy: T::Reflector::reflect(obj) returns
  // a Reflector struct, and T::Reflector::create(reflector) round-trips back.
  //
  // For Lua we use sol::property() getters/setters that go through the
  // reflect/create round-trip so writes are properly committed.
  // ============================================================================

  template <typename T>
  void bind_reflected_custom(sol::state& lua) {
    if (is_bound<T>()) return;
    mark_bound<T>();

    using Reflector = typename T::Reflector;
    std::string name = short_name(ReflectionLibrary::ReflectionInfo<T>::class_name);

    // Phase 1: recursively bind all Lua-safe field types in the Reflector
    std::apply([&](auto&&... field_pairs) {
      (([&]() {
        using FieldT = unwrap_t<member_value_t<decltype(field_pairs.second)>>;
        if constexpr (is_lua_safe_field_v<FieldT>) {
          bind_field_type_if_needed<FieldT>(lua);
        }
      }()),
       ...);
    }, ReflectionLibrary::ReflectionInfo<T>::fields);

    // Phase 2: create usertype with property getters/setters (Lua-safe fields only)
    auto ut = lua.new_usertype<T>(name, sol::constructors<T()>());

    std::apply([&](auto&&... field_pairs) {
      (([&]() {
        using FieldT = unwrap_t<member_value_t<decltype(field_pairs.second)>>;
        if constexpr (is_lua_safe_field_v<FieldT>) {
          ut[field_pairs.first] = sol::property(
            // Getter: reflect -> read field
            [ptr = field_pairs.second](const T& obj) {
              auto reflector = Reflector::reflect(obj);
              return reflector.*(ptr);
            },
            // Setter: reflect -> write field -> create
            [ptr = field_pairs.second](T& obj, const FieldT& val) {
              auto reflector = Reflector::reflect(obj);
              reflector.*(ptr) = val;
              obj = Reflector::create(reflector);
            }
          );
        }
      }()),
       ...);
    }, ReflectionLibrary::ReflectionInfo<T>::fields);
  }

  // ============================================================================
  // bind_reflected_enum<E> — auto-binds a REFLECT_ENUM type as a Lua table
  //
  // Creates a global table with enum values as integer constants:
  //   EasingType.Linear = 0
  //   EasingType.EaseInQuad = 1
  //   etc.
  // ============================================================================

  template <typename T>
    requires ReflectionLibrary::is_reflected_enum_v<T>
  void bind_reflected_enum(sol::state& lua) {
    if (is_bound<T>()) return;
    mark_bound<T>();

    std::string name = short_name(ReflectionLibrary::EnumInfo<T>::enum_name);
    sol::table tbl = lua.create_named_table(name);

    std::apply([&](auto&&... pairs) {
      ((tbl[pairs.second] = static_cast<int>(pairs.first)), ...);
    }, ReflectionLibrary::EnumInfo<T>::values);
  }

} // namespace Engine::LuaBindings
