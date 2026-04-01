local PLAYER_HEARTBEAT_EVENT = "event:/SFX/PlayerHeartBeat"
local PAUSE_DIMMER_NAME = "PauseMenuDimmer"
local PAUSE_CONTROLLER_NAME = "PauseMenuController"
local LEFT_PANEL_NAME = "PauseLeftPanel"
local OPTIONS_LABELS_NAME = "PauseOptionsLabels"
local OPTIONS_BGM_BAR_NAME = "PauseOptionsBGMBar"
local OPTIONS_SFX_BAR_NAME = "PauseOptionsSFXBar"
local OPTIONS_SCREEN_STATE_NAME = "PauseOptionsScreenState"
local OPTIONS_HOVER_OVERLAY_NAME = "PauseOptionsHoverOverlay"
local CONFIRM_PROMPT_NAME = "PauseConfirmPrompt"
local CONFIRM_QUIT_YES_NAME = "PauseConfirmQuitYes"
local CONFIRM_QUIT_NO_NAME = "PauseConfirmQuitNo"

local RESUME_BUTTON_NAME = "PauseResumeButton"
local RESTART_BUTTON_NAME = "PauseRestartButton"
local OPTIONS_BUTTON_NAME = "PauseOptionsButton"
local EXIT_BUTTON_NAME = "PauseExitButton"
local BGM_DECREASE_BUTTON_NAME = "PauseBGMDecreaseButton"
local BGM_INCREASE_BUTTON_NAME = "PauseBGMIncreaseButton"
local SFX_DECREASE_BUTTON_NAME = "PauseSFXDecreaseButton"
local SFX_INCREASE_BUTTON_NAME = "PauseSFXIncreaseButton"
local WINDOWED_BUTTON_NAME = "PauseWindowedButton"
local FULLSCREEN_BUTTON_NAME = "PauseFullscreenButton"
local CONFIRM_YES_BUTTON_NAME = "PauseConfirmYesButton"
local CONFIRM_NO_BUTTON_NAME = "PauseConfirmNoButton"

local MENU_SCROLL_NAMES = { "PausePausedScrollA", "PausePausedScrollB" }
local RESTART_YES_SCROLL_NAMES = { "PauseRestartYesScrollA", "PauseRestartYesScrollB" }
local RESTART_NO_SCROLL_NAMES = { "PauseRestartNoScrollA", "PauseRestartNoScrollB" }

local FULLSCREEN_CENTER_X = 800.0
local FULLSCREEN_CENTER_Y = 450.0
local SCROLL_HEIGHT = 1800.0
local SCROLL_SPEED = 90.0
local VOLUME_STEPS = 10

local LEFT_PANEL_TEXTURES = {
  menu = "images/UI/In-Game Pause/States/NothingSelectedPausedMenu.png",
  resume = "images/UI/In-Game Pause/States/ResumeButton.png",
  restart = "images/UI/In-Game Pause/States/RestartButton.png",
  options = "images/UI/In-Game Pause/States/OptionsButtons.png",
  quit = "images/UI/In-Game Pause/States/ExitButton.png",
}

local OPTIONS_TEXTURES = {
  labels = "images/UI/In-Game Pause/Options/BGMSFXSCREEN.png",
  bgm_bar_prefix = "images/UI/In-Game Pause/Options/BGM/BGMVolumeBar/BGM",
  sfx_bar_prefix = "images/UI/In-Game Pause/Options/SFX/SFXVolume/SFX",
  bgm_decrease = "images/UI/In-Game Pause/Options/BGM/BGMVolumeDecrease.png",
  bgm_increase = "images/UI/In-Game Pause/Options/BGM/BGMVolumeIncrease.png",
  sfx_decrease = "images/UI/In-Game Pause/Options/SFX/SFXVolumeDecrease.png",
  sfx_increase = "images/UI/In-Game Pause/Options/SFX/SFXVolumeIncrease.png",
  screen_left = "images/UI/In-Game Pause/Options/Screen/LeftArrowScreen.png",
  screen_right = "images/UI/In-Game Pause/Options/Screen/RightArrowScreen.png",
  windowed = "images/UI/In-Game Pause/Options/Screen/WindowedSelected.png",
  fullscreen = "images/UI/In-Game Pause/Options/Screen/FullScreenSelected.png",
}

local CONFIRM_TEXTURES = {
  prompt = "images/UI/In-Game Pause/QuitGame/ARE YOU SURE.png",
  quit_yes_default = "images/UI/In-Game Pause/QuitGame/YesNotSelected.png",
  quit_yes_hover = "images/UI/In-Game Pause/QuitGame/YesHover.png",
  quit_no_default = "images/UI/In-Game Pause/QuitGame/NoNotSelected.png",
  quit_no_hover = "images/UI/In-Game Pause/QuitGame/NoHover.png",
  restart_yes_default = "images/UI/In-Game Pause/RESTART/YesNotSelected.png",
  restart_yes_hover = "images/UI/In-Game Pause/RESTART/YesHover.png",
  restart_no_default = "images/UI/In-Game Pause/RESTART/NoGray.png",
  restart_no_hover = "images/UI/In-Game Pause/RESTART/NoHover.png",
  paused_scroll = "images/UI/In-Game Pause/RESUMEGAME/PAUSED.png",
}

local RIGHT_PANEL_NAMES = {
  OPTIONS_LABELS_NAME,
  OPTIONS_BGM_BAR_NAME,
  OPTIONS_SFX_BAR_NAME,
  OPTIONS_SCREEN_STATE_NAME,
  OPTIONS_HOVER_OVERLAY_NAME,
  CONFIRM_PROMPT_NAME,
  CONFIRM_QUIT_YES_NAME,
  CONFIRM_QUIT_NO_NAME,
  MENU_SCROLL_NAMES[1],
  MENU_SCROLL_NAMES[2],
  RESTART_YES_SCROLL_NAMES[1],
  RESTART_YES_SCROLL_NAMES[2],
  RESTART_NO_SCROLL_NAMES[1],
  RESTART_NO_SCROLL_NAMES[2],
}

local LEFT_BUTTON_NAMES = {
  RESUME_BUTTON_NAME,
  RESTART_BUTTON_NAME,
  OPTIONS_BUTTON_NAME,
  EXIT_BUTTON_NAME,
}

local OPTIONS_BUTTON_NAMES = {
  BGM_DECREASE_BUTTON_NAME,
  BGM_INCREASE_BUTTON_NAME,
  SFX_DECREASE_BUTTON_NAME,
  SFX_INCREASE_BUTTON_NAME,
  WINDOWED_BUTTON_NAME,
  FULLSCREEN_BUTTON_NAME,
}

local CONFIRM_BUTTON_NAMES = {
  CONFIRM_YES_BUTTON_NAME,
  CONFIRM_NO_BUTTON_NAME,
}

local ENTITY_LAYER_NAMES = {
  [PAUSE_DIMMER_NAME] = "overlay_bg",
  [LEFT_PANEL_NAME] = "overlay_items_2",
  [OPTIONS_LABELS_NAME] = "overlay_items_2",
  [OPTIONS_BGM_BAR_NAME] = "overlay_items_2",
  [OPTIONS_SFX_BAR_NAME] = "overlay_items_2",
  [OPTIONS_SCREEN_STATE_NAME] = "overlay_items_2",
  [OPTIONS_HOVER_OVERLAY_NAME] = "overlay_items_3",
  [CONFIRM_PROMPT_NAME] = "overlay_items_2",
  [CONFIRM_QUIT_YES_NAME] = "overlay_items_3",
  [CONFIRM_QUIT_NO_NAME] = "overlay_items_3",
  [MENU_SCROLL_NAMES[1]] = "overlay_items_1",
  [MENU_SCROLL_NAMES[2]] = "overlay_items_1",
  [RESTART_YES_SCROLL_NAMES[1]] = "overlay_items_1",
  [RESTART_YES_SCROLL_NAMES[2]] = "overlay_items_1",
  [RESTART_NO_SCROLL_NAMES[1]] = "overlay_items_1",
  [RESTART_NO_SCROLL_NAMES[2]] = "overlay_items_1",
  [RESUME_BUTTON_NAME] = "overlay_items_3",
  [RESTART_BUTTON_NAME] = "overlay_items_3",
  [OPTIONS_BUTTON_NAME] = "overlay_items_3",
  [EXIT_BUTTON_NAME] = "overlay_items_3",
  [BGM_DECREASE_BUTTON_NAME] = "overlay_items_3",
  [BGM_INCREASE_BUTTON_NAME] = "overlay_items_3",
  [SFX_DECREASE_BUTTON_NAME] = "overlay_items_3",
  [SFX_INCREASE_BUTTON_NAME] = "overlay_items_3",
  [WINDOWED_BUTTON_NAME] = "overlay_items_3",
  [FULLSCREEN_BUTTON_NAME] = "overlay_items_3",
  [CONFIRM_YES_BUTTON_NAME] = "overlay_items_3",
  [CONFIRM_NO_BUTTON_NAME] = "overlay_items_3",
}

local ENTITY_VISIBLE_ALPHA = {
  [PAUSE_DIMMER_NAME] = 0.5,
}

local HITBOXES = {
  resume = { left = -504.802399, top = -354.316132, right = -51.975685, bottom = -165.866135 },
  restart = { left = -522.644531, top = -158.237122, right = -54.62006, bottom = 15.015185 },
  options = { left = -521.763062, top = -43.130722, right = -57.264435, bottom = 209.149017 },
  quit = { left = -527.447083, top = 63.829796, right = -53.829784, bottom = 386.018311 },
  bgm_decrease = { left = -115.833, top = -218.333, right = -67.5, bottom = -138.333 },
  bgm_increase = { left = 709.167, top = -219.167, right = 757.5, bottom = -139.167 },
  sfx_decrease = { left = -116.667, top = 22.5, right = -68.333, bottom = 102.5 },
  sfx_increase = { left = 708.333, top = 21.667, right = 756.667, bottom = 101.667 },
  windowed = { left = -115.833, top = 260.0, right = -67.5, bottom = 340.0 },
  fullscreen = { left = 705.0, top = 260.0, right = 753.333, bottom = 340.0 },
  confirm_yes = { left = -337.5, top = 45.0, right = -102.5, bottom = 159.167 },
  confirm_no = { left = 125.833, top = 45.0, right = 297.5, bottom = 159.167 },
}

local LEFT_HOVER_TARGETS = {
  { key = "resume", rect = HITBOXES.resume },
  { key = "restart", rect = HITBOXES.restart },
  { key = "options", rect = HITBOXES.options },
  { key = "quit", rect = HITBOXES.quit },
}

local OPTIONS_HOVER_TARGETS = {
  { key = OPTIONS_TEXTURES.bgm_decrease, rect = HITBOXES.bgm_decrease },
  { key = OPTIONS_TEXTURES.bgm_increase, rect = HITBOXES.bgm_increase },
  { key = OPTIONS_TEXTURES.sfx_decrease, rect = HITBOXES.sfx_decrease },
  { key = OPTIONS_TEXTURES.sfx_increase, rect = HITBOXES.sfx_increase },
  { key = OPTIONS_TEXTURES.screen_left, rect = HITBOXES.windowed },
  { key = OPTIONS_TEXTURES.screen_right, rect = HITBOXES.fullscreen },
}

local CONFIRM_HOVER_TARGETS = {
  { key = "yes", rect = HITBOXES.confirm_yes },
  { key = "no", rect = HITBOXES.confirm_no },
}

local hovered_left_button = nil
local hovered_options_overlay = nil
local hovered_confirm_button = nil

local function clamp(value, min_value, max_value)
  return math.min(math.max(value, min_value), max_value)
end

local function get_named_entity(name)
  return ECS.get_entity_by_name(name)
end

local function get_pause_controller()
  return get_named_entity(PAUSE_CONTROLLER_NAME)
end

local function get_pause_state_machine()
  local controller = get_pause_controller()
  if not controller or not controller:is_valid() then
    return nil
  end

  return controller.StateMachine
end

local function is_pause_state(state_name)
  local sm = get_pause_state_machine()
  return sm and sm:is_in_state(state_name)
end

local function change_pause_state(state_name)
  local sm = get_pause_state_machine()
  if sm and sm:has_state(state_name) then
    sm:change_state(state_name)
  end
end

local function set_entity_alpha(name, alpha)
  local entity = get_named_entity(name)
  if not entity or not entity:is_valid() then
    return
  end

  local color = entity.Color
  if color then
    color.a = alpha
  end
end

local function set_entity_visible(name, visible)
  local entity = get_named_entity(name)
  if not entity or not entity:is_valid() then
    return
  end

  entity:clear_layers()

  if visible then
    local layer_name = ENTITY_LAYER_NAMES[name]
    if layer_name ~= nil then
      entity:add_to_layer(layer_name)
    else
      entity:add_to_layer("default")
    end
  end

  local color = entity.Color
  if color then
    color.a = visible and (ENTITY_VISIBLE_ALPHA[name] or 1.0) or 0.0
  end
end

local function set_entity_texture(name, path)
  local entity = get_named_entity(name)
  if not entity or not entity:is_valid() then
    return
  end

  local texture = entity.Texture
  if texture then
    texture.path = path
  end
end

local function set_entity_y(name, y)
  local entity = get_named_entity(name)
  if not entity or not entity:is_valid() then
    return
  end

  local tr = entity.Transform2D
  if tr then
    tr.position.y = y
  end
end

local function get_mouse_local_position()
  local mouse = Input.get_mouse_position_if_in_viewport()
  if not mouse then
    return nil
  end

  return {
    x = mouse.x - FULLSCREEN_CENTER_X,
    y = mouse.y - FULLSCREEN_CENTER_Y,
  }
end

local function point_in_rect(point, rect)
  return point.x >= rect.left and point.x <= rect.right
    and point.y >= rect.top and point.y <= rect.bottom
end

local function get_hover_target(entries)
  local mouse = get_mouse_local_position()
  if mouse == nil then
    return nil
  end

  for _, entry in ipairs(entries) do
    if point_in_rect(mouse, entry.rect) then
      return entry.key
    end
  end

  return nil
end

local function get_default_left_panel_texture()
  if is_pause_state("Options") then
    return LEFT_PANEL_TEXTURES.options
  end
  if is_pause_state("ConfirmRestart") then
    return LEFT_PANEL_TEXTURES.restart
  end
  if is_pause_state("ConfirmQuit") then
    return LEFT_PANEL_TEXTURES.quit
  end
  return LEFT_PANEL_TEXTURES.menu
end

local function refresh_left_panel()
  local path = get_default_left_panel_texture()
  if hovered_left_button ~= nil then
    path = LEFT_PANEL_TEXTURES[hovered_left_button] or path
  end

  set_entity_texture(LEFT_PANEL_NAME, path)
end

local set_entities_visible

local function show_base_pause_entities()
  set_entity_visible(PAUSE_DIMMER_NAME, true)
  set_entity_visible(LEFT_PANEL_NAME, true)
  set_entities_visible(LEFT_BUTTON_NAMES, true)
end

local function hide_state_specific_buttons()
  set_entities_visible(OPTIONS_BUTTON_NAMES, false)
  set_entities_visible(CONFIRM_BUTTON_NAMES, false)
end

local function hide_right_panels()
  for _, entity_name in ipairs(RIGHT_PANEL_NAMES) do
    set_entity_visible(entity_name, false)
  end
end

set_entities_visible = function(entity_names, visible)
  for _, entity_name in ipairs(entity_names) do
    set_entity_visible(entity_name, visible)
  end
end

local function update_scroll_pair(entity_names)
  local offset = math.fmod(Time.get_unscaled_time() * SCROLL_SPEED, SCROLL_HEIGHT)
  set_entity_y(entity_names[1], FULLSCREEN_CENTER_Y + offset)
  set_entity_y(entity_names[2], FULLSCREEN_CENTER_Y + offset - SCROLL_HEIGHT)
end

local function volume_to_step(volume)
  return clamp(math.floor(volume * (VOLUME_STEPS - 1) + 1.5), 1, VOLUME_STEPS)
end

local function step_to_volume(step)
  return clamp((step - 1) / (VOLUME_STEPS - 1), 0.0, 1.0)
end

local function get_volume_bar_path(prefix, step)
  return string.format("%s%d.png", prefix, clamp(step, 1, VOLUME_STEPS))
end

local function sync_options_visuals()
  set_entity_texture(OPTIONS_BGM_BAR_NAME, get_volume_bar_path(OPTIONS_TEXTURES.bgm_bar_prefix, volume_to_step(Settings.get_bgm_volume())))
  set_entity_texture(OPTIONS_SFX_BAR_NAME, get_volume_bar_path(OPTIONS_TEXTURES.sfx_bar_prefix, volume_to_step(Settings.get_sfx_volume())))
  set_entity_texture(OPTIONS_SCREEN_STATE_NAME, Settings.get_fullscreen() and OPTIONS_TEXTURES.fullscreen or OPTIONS_TEXTURES.windowed)
end

local function hide_options_hover_overlay()
  hovered_options_overlay = nil
  set_entity_visible(OPTIONS_HOVER_OVERLAY_NAME, false)
end

local function show_options_hover_overlay(path)
  if not is_pause_state("Options") then
    return
  end

  hovered_options_overlay = path
  set_entity_texture(OPTIONS_HOVER_OVERLAY_NAME, path)
  set_entity_visible(OPTIONS_HOVER_OVERLAY_NAME, true)
end

local function reset_confirm_quit_visuals()
  set_entity_texture(CONFIRM_QUIT_YES_NAME, CONFIRM_TEXTURES.quit_yes_default)
  set_entity_texture(CONFIRM_QUIT_NO_NAME, CONFIRM_TEXTURES.quit_no_default)
end

local function reset_confirm_restart_visuals()
  for _, entity_name in ipairs(RESTART_YES_SCROLL_NAMES) do
    set_entity_texture(entity_name, CONFIRM_TEXTURES.restart_yes_default)
  end

  for _, entity_name in ipairs(RESTART_NO_SCROLL_NAMES) do
    set_entity_texture(entity_name, CONFIRM_TEXTURES.restart_no_default)
  end
end

local function apply_confirm_hover(choice)
  hovered_confirm_button = choice

  if is_pause_state("ConfirmQuit") then
    if choice == "yes" then
      set_entity_texture(CONFIRM_QUIT_YES_NAME, CONFIRM_TEXTURES.quit_yes_hover)
      set_entity_texture(CONFIRM_QUIT_NO_NAME, CONFIRM_TEXTURES.quit_no_default)
    elseif choice == "no" then
      set_entity_texture(CONFIRM_QUIT_YES_NAME, CONFIRM_TEXTURES.quit_yes_default)
      set_entity_texture(CONFIRM_QUIT_NO_NAME, CONFIRM_TEXTURES.quit_no_hover)
    else
      reset_confirm_quit_visuals()
    end
    return
  end

  if is_pause_state("ConfirmRestart") then
    if choice == "yes" then
      for _, entity_name in ipairs(RESTART_YES_SCROLL_NAMES) do
        set_entity_texture(entity_name, CONFIRM_TEXTURES.restart_yes_hover)
      end
      for _, entity_name in ipairs(RESTART_NO_SCROLL_NAMES) do
        set_entity_texture(entity_name, CONFIRM_TEXTURES.restart_no_default)
      end
    elseif choice == "no" then
      for _, entity_name in ipairs(RESTART_YES_SCROLL_NAMES) do
        set_entity_texture(entity_name, CONFIRM_TEXTURES.restart_yes_default)
      end
      for _, entity_name in ipairs(RESTART_NO_SCROLL_NAMES) do
        set_entity_texture(entity_name, CONFIRM_TEXTURES.restart_no_hover)
      end
    else
      reset_confirm_restart_visuals()
    end
  end
end

local function update_left_hover_from_mouse()
  local next_hover = get_hover_target(LEFT_HOVER_TARGETS)
  if hovered_left_button == next_hover then
    return
  end

  hovered_left_button = next_hover
  refresh_left_panel()
end

local function update_options_hover_from_mouse()
  if not is_pause_state("Options") then
    hide_options_hover_overlay()
    return
  end

  local next_hover = get_hover_target(OPTIONS_HOVER_TARGETS)
  if hovered_options_overlay == next_hover then
    return
  end

  if next_hover == nil then
    hide_options_hover_overlay()
  else
    show_options_hover_overlay(next_hover)
  end
end

local function update_confirm_hover_from_mouse()
  if not is_pause_state("ConfirmRestart") and not is_pause_state("ConfirmQuit") then
    apply_confirm_hover(nil)
    return
  end

  local next_hover = get_hover_target(CONFIRM_HOVER_TARGETS)
  if hovered_confirm_button == next_hover then
    return
  end

  apply_confirm_hover(next_hover)
end

local function show_options_panel()
  set_entity_visible(OPTIONS_LABELS_NAME, true)
  set_entity_visible(OPTIONS_BGM_BAR_NAME, true)
  set_entity_visible(OPTIONS_SFX_BAR_NAME, true)
  set_entity_visible(OPTIONS_SCREEN_STATE_NAME, true)
  hide_options_hover_overlay()
  sync_options_visuals()
end

local function show_confirm_quit_panel()
  set_entity_visible(CONFIRM_PROMPT_NAME, true)
  set_entity_visible(CONFIRM_QUIT_YES_NAME, true)
  set_entity_visible(CONFIRM_QUIT_NO_NAME, true)
  reset_confirm_quit_visuals()
end

local function show_confirm_restart_panel()
  set_entity_visible(CONFIRM_PROMPT_NAME, true)
  set_entities_visible(RESTART_YES_SCROLL_NAMES, true)
  set_entities_visible(RESTART_NO_SCROLL_NAMES, true)
  reset_confirm_restart_visuals()
  update_scroll_pair(RESTART_YES_SCROLL_NAMES)
  update_scroll_pair(RESTART_NO_SCROLL_NAMES)
end

local function show_menu_panel()
  set_entities_visible(MENU_SCROLL_NAMES, true)
  update_scroll_pair(MENU_SCROLL_NAMES)
end

local function set_left_hover(button_name)
  hovered_left_button = button_name
  refresh_left_panel()
end

local function clear_left_hover()
  hovered_left_button = nil
  refresh_left_panel()
end

local function change_bgm_step(delta)
  if not is_pause_state("Options") then
    return
  end

  local step = volume_to_step(Settings.get_bgm_volume()) + delta
  Settings.set_bgm_volume(step_to_volume(clamp(step, 1, VOLUME_STEPS)))
  sync_options_visuals()
end

local function change_sfx_step(delta)
  if not is_pause_state("Options") then
    return
  end

  local step = volume_to_step(Settings.get_sfx_volume()) + delta
  Settings.set_sfx_volume(step_to_volume(clamp(step, 1, VOLUME_STEPS)))
  sync_options_visuals()
end

function pause_menu_resume()
  Scene.pop()
end

function pause_menu_restart()
  if not is_pause_state("ConfirmRestart") then
    change_pause_state("ConfirmRestart")
  end
end

function pause_menu_options()
  if is_pause_state("Options") then
    change_pause_state("Menu")
  else
    change_pause_state("Options")
  end
end

function pause_menu_exit()
  if not is_pause_state("ConfirmQuit") then
    change_pause_state("ConfirmQuit")
  end
end

function pause_bgm_decrease()
  change_bgm_step(-1)
end

function pause_bgm_increase()
  change_bgm_step(1)
end

function pause_sfx_decrease()
  change_sfx_step(-1)
end

function pause_sfx_increase()
  change_sfx_step(1)
end

function pause_set_windowed()
  if not is_pause_state("Options") then
    return
  end

  Settings.set_fullscreen(false)
  sync_options_visuals()
end

function pause_set_fullscreen()
  if not is_pause_state("Options") then
    return
  end

  Settings.set_fullscreen(true)
  sync_options_visuals()
end

function pause_confirm_yes()
  if is_pause_state("ConfirmRestart") then
    local level_id = Level.get_active_id()
    Scene.pop()
    if level_id ~= "" then
      Level.reload(level_id)
    end
    return
  end

  if is_pause_state("ConfirmQuit") then
    Audio.stop_event(PLAYER_HEARTBEAT_EVENT, true)
    Scene.load("MainMenuState")
  end
end

function pause_confirm_no()
  if is_pause_state("ConfirmRestart") or is_pause_state("ConfirmQuit") then
    change_pause_state("Menu")
  end
end

function pause_hover_resume(entity)
end

function pause_hover_restart(entity)
end

function pause_hover_options(entity)
end

function pause_hover_exit(entity)
end

function pause_left_button_exit_hover(entity)
end

function pause_hover_bgm_decrease(entity)
end

function pause_hover_bgm_increase(entity)
end

function pause_hover_sfx_decrease(entity)
end

function pause_hover_sfx_increase(entity)
end

function pause_hover_screen_left(entity)
end

function pause_hover_screen_right(entity)
end

function pause_options_hover_exit(entity)
end

function pause_confirm_yes_hover(entity)
end

function pause_confirm_no_hover(entity)
end

function pause_confirm_hover_exit(entity)
end

function pause_menu_menu_on_enter(entity)
  hovered_left_button = nil
  hovered_options_overlay = nil
  hovered_confirm_button = nil
  show_base_pause_entities()
  hide_right_panels()
  hide_state_specific_buttons()
  show_menu_panel()
  refresh_left_panel()
  update_left_hover_from_mouse()
end

function pause_menu_menu_on_update(entity)
  update_scroll_pair(MENU_SCROLL_NAMES)
  update_left_hover_from_mouse()

  if Input.is_pressed(Key.Escape) then
    pause_menu_resume()
  end
end

function pause_menu_menu_on_exit(entity)
end

function pause_menu_options_on_enter(entity)
  hovered_left_button = nil
  hovered_options_overlay = nil
  hovered_confirm_button = nil
  show_base_pause_entities()
  hide_right_panels()
  hide_state_specific_buttons()
  set_entities_visible(OPTIONS_BUTTON_NAMES, true)
  show_options_panel()
  refresh_left_panel()
  update_left_hover_from_mouse()
  update_options_hover_from_mouse()
end

function pause_menu_options_on_update(entity)
  update_left_hover_from_mouse()
  update_options_hover_from_mouse()

  if Input.is_pressed(Key.Escape) then
    change_pause_state("Menu")
  end
end

function pause_menu_options_on_exit(entity)
  hide_options_hover_overlay()
end

function pause_menu_confirm_restart_on_enter(entity)
  hovered_left_button = nil
  hovered_options_overlay = nil
  hovered_confirm_button = nil
  show_base_pause_entities()
  hide_right_panels()
  hide_state_specific_buttons()
  set_entities_visible(CONFIRM_BUTTON_NAMES, true)
  show_confirm_restart_panel()
  refresh_left_panel()
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()
end

function pause_menu_confirm_restart_on_update(entity)
  update_scroll_pair(RESTART_YES_SCROLL_NAMES)
  update_scroll_pair(RESTART_NO_SCROLL_NAMES)
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()

  if Input.is_pressed(Key.Escape) then
    change_pause_state("Menu")
  end
end

function pause_menu_confirm_restart_on_exit(entity)
end

function pause_menu_confirm_quit_on_enter(entity)
  hovered_left_button = nil
  hovered_options_overlay = nil
  hovered_confirm_button = nil
  show_base_pause_entities()
  hide_right_panels()
  hide_state_specific_buttons()
  set_entities_visible(CONFIRM_BUTTON_NAMES, true)
  show_confirm_quit_panel()
  refresh_left_panel()
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()
end

function pause_menu_confirm_quit_on_update(entity)
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()

  if Input.is_pressed(Key.Escape) then
    change_pause_state("Menu")
  end
end

function pause_menu_confirm_quit_on_exit(entity)
end

register_blueprint("PauseMenu", {
  initial_state = "Menu",
  states = {
    { name = "Menu",           on_enter = "pause_menu_menu_on_enter",            on_update = "pause_menu_menu_on_update",            on_exit = "pause_menu_menu_on_exit" },
    { name = "Options",        on_enter = "pause_menu_options_on_enter",         on_update = "pause_menu_options_on_update",         on_exit = "pause_menu_options_on_exit" },
    { name = "ConfirmRestart", on_enter = "pause_menu_confirm_restart_on_enter", on_update = "pause_menu_confirm_restart_on_update", on_exit = "pause_menu_confirm_restart_on_exit" },
    { name = "ConfirmQuit",    on_enter = "pause_menu_confirm_quit_on_enter",    on_update = "pause_menu_confirm_quit_on_update",    on_exit = "pause_menu_confirm_quit_on_exit" },
  }
})