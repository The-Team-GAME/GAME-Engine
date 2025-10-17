#pragma once

#include <type_traits>
#include <concepts>
#include <array>

// Forward declaration
template <typename T>
struct ReflectionInfo;

template <typename T>
constexpr bool is_reflectable_v = requires { ReflectionInfo<T>::fields; };

// Helper concept to check if a type is a std::tuple
template <typename T>
concept is_tuple_v = requires { std::tuple_size<T>::value; };

// Helper concept to check if a type is a std::pair
template <typename T>
concept is_pair_v = requires { typename T::first_type; typename T::second_type; };

// Helper concept to check if a type is iterable (has size, begin, end)
template <typename T>
concept is_iterable_v = requires(T value) { value.size(); value.begin(); value.end(); };

template <typename T>
concept has_reflector_v = requires { typename T::Reflector; };