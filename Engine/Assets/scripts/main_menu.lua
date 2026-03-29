-- main_menu.lua
-- MainMenu state machine and button callbacks (Lua port of MainMenuCallbacks.cpp)

-- ============================================================================
-- Audio event paths (mirrors MainMenuState.h constants)
-- ============================================================================
local MAIN_MENU_BGM_EVENT    = "event:/BGM/MainMenu"
local SPLASH_SFX_SNORE_EVENT = "event:/SFX/SplashSnoreSFX"
local SPLASH_SFX_TRAIN_EVENT = "event:/SFX/SplashTrainSFX"

-- ============================================================================
-- Helper: check whether any keyboard key or mouse button was pressed this frame.
-- Mirrors the C++ loop: for (int key = 32; key <= 348; ++key)
-- Sol2 coerces Lua integers to Engine::KeyCode automatically.
-- ============================================================================
local function any_key_pressed()
  if Input.is_pressed(Key.MouseLeft) or Input.is_pressed(Key.MouseRight) or Input.is_pressed(Key.MouseMiddle) then
    return true
  end
  for key = 32, 348 do
    if Input.is_pressed(key) then
      return true
    end
  end
  return false
end

-- ============================================================================
-- "Start" state — sleeping / title screen
-- ============================================================================

function main_menu_start_on_enter(entity)
  -- nothing needed; buttons are held off-screen by the Start state property animation
end

function main_menu_start_on_update(entity)
  if any_key_pressed() then
    local sm = entity.StateMachine
    if sm then
      sm:change_state("Transition")
    end
  end
end

function main_menu_start_on_exit(entity)
  -- Stop splash SFX and start the main menu BGM
  Audio.stop_event(SPLASH_SFX_SNORE_EVENT, true)
  Audio.stop_event(SPLASH_SFX_TRAIN_EVENT, true)
  Audio.play_scene_music(MAIN_MENU_BGM_EVENT, 0.3)
end

-- ============================================================================
-- Sprite-swap callbacks: no longer needed (Carriage-Sheet handles transitions)
-- ============================================================================

function main_menu_bro_menu_enter(entity)
  -- no-op: bro is now part of Carriage-Sheet.png
end

function main_menu_front_menu_enter(entity)
  -- no-op: carriage is now part of Carriage-Sheet.png
end

-- ============================================================================
-- "Transition" state — buttons fly in from the right
-- ============================================================================

function main_menu_transition_on_enter(entity)
  -- nothing needed; fly-in is driven by the Transition state property animation
end

function main_menu_transition_on_update(entity)
  -- Advance to Menu once the fly-in property animation has finished.
  -- Entities with no Transition property animation (train, etc.) will have
  -- is_property_anim_playing() = false immediately and skip through instantly.
  local sm = entity.StateMachine
  if sm and not sm:is_property_anim_playing() then
    sm:change_state("Menu")
  end
end

function main_menu_transition_on_exit(entity)
  -- nothing needed
end

-- ============================================================================
-- "Menu" state — main menu buttons visible
-- ============================================================================

function main_menu_menu_on_enter(entity)
  -- nothing needed; buttons have arrived at their resting positions
end

function main_menu_menu_on_update(entity)
  -- nothing needed; buttons are handled by their Clickable components
end

function main_menu_menu_on_exit(entity)
  -- nothing needed
end

-- ============================================================================
-- Button hover / exit callbacks
-- These are top-level globals so the post-load global scan registers them
-- into FunctionRegistry automatically — no explicit register call needed.
-- ============================================================================

-- Play button
function main_menu_play_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/PlayButtonHovered.PNG"
  end
end

function main_menu_play_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/PlayButton.png"
  end
end

-- Options button
function main_menu_options_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/OptionsButtonHovered.PNG"
  end
end

function main_menu_options_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/OptionsButton.png"
  end
end

-- Credits button
function main_menu_credits_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/CreditsButtonHovered.PNG"
  end
end

function main_menu_credits_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/CreditsButton.png"
  end
end

-- Exit button
function main_menu_exit_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/ExitButtonHovered.PNG"
  end
end

function main_menu_exit_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/NewMainMenu/Buttons/ExitButton.png"
  end
end

-- ============================================================================
-- Blueprint registration
-- ============================================================================

register_blueprint("MainMenu", {
  initial_state = "Start",
  states = {
    { name = "Start",      on_enter = "main_menu_start_on_enter",      on_update = "main_menu_start_on_update",      on_exit = "main_menu_start_on_exit" },
    { name = "Transition", on_enter = "main_menu_transition_on_enter", on_update = "main_menu_transition_on_update", on_exit = "main_menu_transition_on_exit" },
    { name = "Menu",       on_enter = "main_menu_menu_on_enter",       on_update = "main_menu_menu_on_update",       on_exit = "main_menu_menu_on_exit" },
  }
})
