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

-- Stores per-button fly-in animation state keyed by entity ID.
-- { start_x, start_y, target_x, target_y, start_time, delay, duration }
local button_anim = {}

-- The reworked MainMenu.json keeps the buttons authored off-screen.
-- These are the historical on-screen resting positions that the buttons
-- should fly toward during the Transition state.
local BUTTON_TARGETS = {
  ["images/NewMainMenu/Buttons/PlayButton.png"] = { x = 665.83, y = 294.58, delay = 0.00 },
  ["images/NewMainMenu/Buttons/OptionsButton.png"] = { x = 989.17, y = 346.67, delay = 0.07 },
  ["images/NewMainMenu/Buttons/CreditsButton.png"] = { x = 1054.58, y = 542.50, delay = 0.14 },
  ["images/NewMainMenu/Buttons/ExitButton.png"] = { x = 925.42, y = 754.58, delay = 0.21 },
}

local SLIDE_DURATION = 0.5

local function ease_out_cubic(t)
  local u = 1 - t
  return 1 - u * u * u
end

local function get_button_target(entity)
  local tex = entity.Texture
  if not tex then
    return nil
  end

  return BUTTON_TARGETS[tex.path]
end

-- ============================================================================
-- "Start" state — sleeping / title screen
-- ============================================================================

function main_menu_start_on_enter(entity)
  -- Button transforms are already authored off-screen in MainMenu.json.
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
  if not entity:has("Clickable") then
    return
  end

  local tr = entity.Transform2D
  local target = get_button_target(entity)
  if not tr or not target then
    return
  end

  local color = entity.Color
  if color then
    color.a = 1.0
  end

  button_anim[entity:get_id()] = {
    start_x = tr.position.x,
    start_y = tr.position.y,
    target_x = target.x,
    target_y = target.y,
    start_time = Time.get_time(),
    delay = target.delay,
    duration = SLIDE_DURATION,
  }
end

function main_menu_transition_on_update(entity)
  if entity:has("Clickable") then
    local eid = entity:get_id()
    local anim = button_anim[eid]
    local sm = entity.StateMachine
    if not anim then
      if sm then
        sm:change_state("Menu")
      end
      return
    end

    local elapsed = Time.get_time() - anim.start_time - anim.delay
    if elapsed < 0.0 then
      local color = entity.Color
      if color then
        color.a = 1.0
      end
      return
    end

    local t = math.min(elapsed / anim.duration, 1.0)
    local eased = ease_out_cubic(t)
    local tr = entity.Transform2D
    if tr then
      tr.position.x = anim.start_x + (anim.target_x - anim.start_x) * eased
      tr.position.y = anim.start_y + (anim.target_y - anim.start_y) * eased
    end

    local color = entity.Color
    if color then
      color.a = 1.0
    end

    if t >= 1.0 then
      button_anim[eid] = nil
      if sm then
        sm:change_state("Menu")
      end
    end
    return
  end

  -- Non-button entities still rely on their authored property animations.
  local sm = entity.StateMachine
  if sm and not sm:is_property_anim_playing() then
    sm:change_state("Menu")
  end
end

function main_menu_transition_on_exit(entity)
  if entity:has("Clickable") then
    button_anim[entity:get_id()] = nil
  end
end

-- ============================================================================
-- "Menu" state — main menu buttons visible
-- ============================================================================

function main_menu_menu_on_enter(entity)
  if not entity:has("Clickable") then
    return
  end

  local tr = entity.Transform2D
  local target = get_button_target(entity)
  if tr and target then
    tr.position.x = target.x
    tr.position.y = target.y
  end

  local color = entity.Color
  if color then
    color.a = 1.0
  end
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
