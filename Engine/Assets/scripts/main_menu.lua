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
-- { target_x, start_x, start_time, delay, duration }
local button_anim = {}

-- Stores each button's resting X (from JSON) so menu_on_enter can use it
-- as the slide target even though the button is already off-screen.
local button_rest_x = {}

local SLIDE_OFFSET   = 1800   -- px right of final position (off-screen start)
local SLIDE_DURATION = 0.5    -- seconds for the full slide
local SLIDE_STAGGER  = 0.07   -- extra delay per button rank (top-to-bottom)

-- Assigns a stagger delay based on vertical position rank.
-- Buttons are roughly ordered Play(~295) → Options(~347) → Credits(~543) → Exit(~755).
local function slide_delay(y)
  if     y < 320 then return 0 * SLIDE_STAGGER
  elseif y < 450 then return 1 * SLIDE_STAGGER
  elseif y < 650 then return 2 * SLIDE_STAGGER
  else                return 3 * SLIDE_STAGGER
  end
end

-- Ease-out cubic: fast departure, smooth landing.
local function ease_out_cubic(t)
  local u = 1 - t
  return 1 - u * u * u
end

-- ============================================================================
-- "Start" state — sleeping / title screen
-- ============================================================================

function main_menu_start_on_enter(entity)
  -- Move button entities off-screen to the right immediately so they are
  -- invisible during the Start state and ready to fly in on Menu enter.
  if not entity:has("Clickable") then return end
  local tr = entity.Transform2D
  if not tr then return end

  local eid           = entity:get_id()
  button_rest_x[eid]  = tr.position.x          -- save JSON resting position
  tr.position.x       = tr.position.x + SLIDE_OFFSET
end

function main_menu_start_on_update(entity)
  if any_key_pressed() then
    local sm = entity.StateMachine
    if sm then
      sm:change_state("Menu")
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
-- Sprite-swap callbacks: called on_enter of "Menu" state for specific entities
-- ============================================================================

-- Entity 49 (player_sleeping): swap sleeping spritesheet → awake Bro.png
function main_menu_bro_menu_enter(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/Bro.png"
  end
end

-- Entity 51 (Train Overlay / Front): swap graffiti → no graffiti version
function main_menu_front_menu_enter(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu (Start)/Front - no graffiti.png"
  end
end

-- ============================================================================
-- "Menu" state — main menu buttons visible
-- ============================================================================

function main_menu_menu_on_enter(entity)
  -- Only animate button entities (those with a Clickable component).
  if not entity:has("Clickable") then return end
  local tr = entity.Transform2D
  if not tr then return end

  local eid      = entity:get_id()
  local target_x = button_rest_x[eid] or tr.position.x - SLIDE_OFFSET
  local start_x  = tr.position.x  -- already off-screen from start_on_enter

  button_anim[eid] = {
    target_x   = target_x,
    start_x    = start_x,
    start_time = Time.get_time(),
    delay      = slide_delay(tr.position.y),
    duration   = SLIDE_DURATION,
  }
end

function main_menu_menu_on_update(entity)
  local eid  = entity:get_id()
  local anim = button_anim[eid]
  if not anim then return end

  local elapsed = Time.get_time() - anim.start_time - anim.delay
  if elapsed < 0 then return end  -- still in stagger delay

  local t     = math.min(elapsed / anim.duration, 1.0)
  local eased = ease_out_cubic(t)
  local tr    = entity.Transform2D
  if tr then
    tr.position.x = anim.start_x + (anim.target_x - anim.start_x) * eased
  end

  if t >= 1.0 then
    button_anim[eid] = nil
  end
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
    tex.path = "images/UI/MainMenu/UI/Hovered/PlayButtonHovered.PNG"
  end
end

function main_menu_play_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/PlayButton.png"
  end
end

-- Options button
function main_menu_options_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/Hovered/OptionsButtonHovered.PNG"
  end
end

function main_menu_options_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/OptionsButton.png"
  end
end

-- Credits button
function main_menu_credits_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/Hovered/CreditsButtonHovered.PNG"
  end
end

function main_menu_credits_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/CreditsButton.png"
  end
end

-- Exit button
function main_menu_exit_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/Hovered/ExitButtonHovered.PNG"
  end
end

function main_menu_exit_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = "images/UI/MainMenu/UI/ExitButton.png"
  end
end

-- ============================================================================
-- Blueprint registration
-- ============================================================================

register_blueprint("MainMenu", {
  initial_state = "Start",
  states = {
    { name = "Start", on_enter = "main_menu_start_on_enter", on_update = "main_menu_start_on_update", on_exit = "main_menu_start_on_exit" },
    { name = "Menu",  on_enter = "main_menu_menu_on_enter",  on_update = "main_menu_menu_on_update",  on_exit = "main_menu_menu_on_exit" },
  }
})
