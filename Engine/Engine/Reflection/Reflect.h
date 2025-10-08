#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <iostream>
#include <type_traits>

#define FIELD_PAIR(CLASS, FIELD) std::make_pair(#FIELD, &CLASS::FIELD)

// Count arguments (supports up to 20)
#define COUNT_ARGS(...) COUNT_ARGS_IMPL(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define COUNT_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N

// Concatenation helper
#define CONCAT(A, B) CONCAT_IMPL(A, B)
#define CONCAT_IMPL(A, B) A##B

// Expand fields based on count
#define EXPAND_FIELDS(CLASS, N, ...) CONCAT(FIELDS_EXPAND_, N)(CLASS, __VA_ARGS__)

#define FIELDS_EXPAND_1(C, F1) FIELD_PAIR(C, F1)
#define FIELDS_EXPAND_2(C, F1, F2) FIELDS_EXPAND_1(C, F1), FIELD_PAIR(C, F2)
#define FIELDS_EXPAND_3(C, F1, F2, F3) FIELDS_EXPAND_2(C, F1, F2), FIELD_PAIR(C, F3)
#define FIELDS_EXPAND_4(C, F1, F2, F3, F4) FIELDS_EXPAND_3(C, F1, F2, F3), FIELD_PAIR(C, F4)
#define FIELDS_EXPAND_5(C, F1, F2, F3, F4, F5) FIELDS_EXPAND_4(C, F1, F2, F3, F4), FIELD_PAIR(C, F5)
#define FIELDS_EXPAND_6(C, F1, F2, F3, F4, F5, F6) FIELDS_EXPAND_5(C, F1, F2, F3, F4, F5), FIELD_PAIR(C, F6)
#define FIELDS_EXPAND_7(C, F1, F2, F3, F4, F5, F6, F7) FIELDS_EXPAND_6(C, F1, F2, F3, F4, F5, F6), FIELD_PAIR(C, F7)
#define FIELDS_EXPAND_8(C, F1, F2, F3, F4, F5, F6, F7, F8) FIELDS_EXPAND_7(C, F1, F2, F3, F4, F5, F6, F7), FIELD_PAIR(C, F8)
// Add more as needed...

// Forward declare the template
template <typename T>
struct reflection_info;

template<typename T, typename = void>
struct has_reflection : std::false_type {};

template<typename T>
struct has_reflection<T, std::void_t<decltype(reflection_info<T>::class_name)>> : std::true_type {};

template<typename T>
constexpr bool has_reflection_v = has_reflection<T>::value;

// Adaptor template (to be specialized)
template<typename T>
struct reflection_adaptor;

// Adaptor traits - check if serialize method exists
template<typename T, typename = void>
struct has_adaptor : std::false_type {};

template<typename T>
struct has_adaptor<T, std::void_t<decltype(reflection_adaptor<T>::serialize(std::declval<T>()))>> : std::true_type {};

template<typename T>
constexpr bool has_adaptor_v = has_adaptor<T>::value;

#define REFLECT(CLASS_NAME, ...)                                                     \
  template <>                                                                        \
  struct reflection_info<CLASS_NAME>                                                 \
  {                                                                                  \
    static constexpr const char* class_name = #CLASS_NAME;                           \
    static constexpr auto fields = std::make_tuple(EXPAND_FIELDS(CLASS_NAME, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)); \
  };

#define REFLECT_ADAPTOR(CLASS_NAME, VIEW_TYPE, SERIALIZE_FUNC, DESERIALIZE_FUNC) \
  template<>                                                                     \
  struct reflection_adaptor<CLASS_NAME>                                          \
  {                                                                              \
    using view_type = VIEW_TYPE;                                                 \
    static constexpr const char* class_name = #CLASS_NAME;                       \
    static view_type serialize(const CLASS_NAME& obj) {                          \
      return obj.SERIALIZE_FUNC();                                               \
    }                                                                            \
    static CLASS_NAME deserialize(const view_type& view) {                       \
      CLASS_NAME obj;                                                            \
      return obj.DESERIALIZE_FUNC(view);                                         \
    }                                                                            \
  };

#include "Helpers.h"