-- LazyMonkey Blueprint
-- Maps state names to Lua callback function names defined in lazy_monkey.lua

Blueprint = {
  initial_state = "Idle",
  states = {
    { name = "Idle",      on_enter = "lazy_monkey_idle_on_enter",      on_update = "lazy_monkey_idle_on_update",      on_exit = "" },
    { name = "Chasing",   on_enter = "lazy_monkey_chasing_on_enter",   on_update = "lazy_monkey_chasing_on_update",   on_exit = "lazy_monkey_chasing_on_exit" },
    { name = "Windup",    on_enter = "lazy_monkey_windup_on_enter",    on_update = "lazy_monkey_windup_on_update",    on_exit = "" },
    { name = "Attacking", on_enter = "lazy_monkey_attacking_on_enter", on_update = "lazy_monkey_attacking_on_update", on_exit = "lazy_monkey_attacking_on_exit" },
    { name = "Hit",       on_enter = "lazy_monkey_hit_on_enter",       on_update = "lazy_monkey_hit_on_update",       on_exit = "lazy_monkey_hit_on_exit" },
    { name = "Death",     on_enter = "lazy_monkey_death_on_enter",     on_update = "lazy_monkey_death_on_update",     on_exit = "lazy_monkey_death_on_exit" },
  }
}
