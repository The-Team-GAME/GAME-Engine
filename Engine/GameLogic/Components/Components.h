#include "../../Engine/MonkeyEngine.h"

namespace Components
{
  struct Health {
    s32 value, max;
    Health(s32 value = 100, s32 max = 100) : value(value), max(max) {}

    void heal(s32 amount) {
      value = std::min(value + amount, max);
    }

    void damage(s32 amount) {
      value = std::max(value - amount, 0);
    }

    bool is_dead() const {
      return value <= 0;
    }
  };
}

REFLECT(Components::Health, value, max)