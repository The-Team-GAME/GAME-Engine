-- Simple test script
print("=== Lua Script Loaded ===")
print("Game Version: " .. GAME_VERSION)

function test(dt)
  log_message("Lua update called! DT: " .. dt)
end

log_message("Lua script ready!")