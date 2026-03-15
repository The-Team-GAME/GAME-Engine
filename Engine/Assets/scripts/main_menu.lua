-- main_menu.lua
-- MainMenu state machine and button callbacks (Lua port of MainMenuCallbacks.cpp)

-- ============================================================================
-- Audio event paths (mirrors MainMenuState.h constants)
-- ============================================================================
local MAIN_MENU_BGM_EVENT       = "event:/BGM/MainMenu"
local SPLASH_SFX_SNORE_EVENT    = "event:/SFX/SplashSnoreSFX"
local SPLASH_SFX_TRAIN_EVENT    = "event:/SFX/SplashTrainSFX"

-- ============================================================================
-- Helper: check whether any keyboard key was pressed this frame.
-- Mirrors the C++ loop: for (int key = 32; key <= 348; ++key)
-- Sol2 coerces Lua integers to Engine::KeyCode automatically.
-- ============================================================================
local function any_key_pressed()
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
  -- nothing needed; property_animation and visual setup are data-driven
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
  -- nothing needed
end

-- ============================================================================
-- "Menu" state — main menu buttons visible
-- ============================================================================

function main_menu_menu_on_enter(entity)
  -- nothing needed; property_animation handles the visual transition
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
    { name = "Menu",  on_enter = "main_menu_menu_on_enter",  on_update = "main_menu_menu_on_update",  on_exit = "main_menu_menu_on_exit"  },
  }
})
