#pragma once

namespace Components
{
  /* ======================================= //
  // Fixed string for compile-time tags     //
  // ======================================= */
  template<size_t N>
  struct fixed_string {
    constexpr fixed_string(const char(&str)[N]) {
      std::copy_n(str, N, value);
    }
    char value[N];
  };

  template <fixed_string id>
  struct Tag {};

}