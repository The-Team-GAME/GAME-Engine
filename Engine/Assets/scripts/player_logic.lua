-- Simple test script using Game API
print("=== Lua Script Loaded ===")
print("Game Version: " .. GAME_VERSION)

-- persistent handle visible to other Lua functions
local player = nil

function OnGameStart()
  player = Game.create_entity_proxy()
  player:add_transform(400, 300, 0.0, 2.0, 2.0)
  player:add_texture("images/player.png")
  player:add_color(1.0, 1.0, 1.0, 1.0)
  log_message("Player entity created from Lua")
end

function test(dt)
  -- if not player then return end
  -- example per-frame logic
  -- e.g. player:add_transform(...) or move via Game.add_component("Transform", {...})
  log_message("Lua update called! DT: " .. dt)
end

log_message("Lua script ready!")