#pragma once

template<size_t N>
struct fixed_string {
  constexpr fixed_string(const char(&str)[N]) {
    std::copy_n(str, N, value);
  }
  char value[N];
};


namespace Components {
  template <fixed_string id>
  struct Tag {};

  struct Position {
    float x, y;
    Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
  };

  struct Velocity {
    float x, y;
    Velocity(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
  };

  struct Health {
    int value, max;
    Health(int value = 100, int max = 100) : value(value), max(max) {}

    void heal(int amount) {
      value = std::min(value + amount, max);
    }

    void damage(int amount) {
      value = std::max(value - amount, 0);
    }

    bool isDead() const {
      return value <= 0;
    }
  };

  struct Renderable {
    /*int spriteId;
    float scale;
    Renderable(int id = 0, float s = 1.0f) : spriteId(id), scale(s) {}*/
  };

} // namespace Components