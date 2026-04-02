-- main_menu.lua
-- MainMenu state machine and button callbacks

local MAIN_MENU_BGM_EVENT    = "event:/BGM/MainMenu"
local SPLASH_SFX_SNORE_EVENT = "event:/SFX/SplashSnoreSFX"
local SPLASH_SFX_TRAIN_EVENT = "event:/SFX/SplashTrainSFX"

local OPTIONS_CONTROLLER_NAME = "Whole train"
local OPTIONS_CAT_PREFAB_PATH = "prefabs/cat_options_idle.json"
local MAIN_MENU_OPTIONS_CAT_NAME = "MainMenuOptionsCat"

local button_anim = {}
local menu_anim = {}
local options_active = false
local options_transition = nil
local options_cat_prefab = nil

local MENU_BUTTON_NAMES = {
  "PlayButton",
  "OptionsButton",
  "CreditsButton",
  "ExitButton",
}

local MENU_BUTTON_INDEX_BY_NAME = {
  PlayButton = 1,
  OptionsButton = 2,
  CreditsButton = 3,
  ExitButton = 4,
}

local OPTION_ROW_ENTITIES = {
  { "OptionsBGMLabel", "OptionsBGMBar", "OptionsBGMLeft", "OptionsBGMRight" },
  { "OptionsSFXLabel", "OptionsSFXBar", "OptionsSFXLeft", "OptionsSFXRight" },
  { "OptionsScreenLabel", "OptionsScreenLeft", "OptionsWindowed", "OptionsFullscreen", "OptionsScreenRight" },
}

local OPTION_ROW_FOCUSED_ALPHA = 1.0
local OPTION_ROW_UNFOCUSED_ALPHA = 0.72

local selected_menu_button_index = 1
local hovered_menu_button_index = nil
local selected_option_row_index = 1

local MAIN_MENU_ENTITY_NAMES = {
  "Whole train",
  "BackCarriage",
  "Carriage",
  "BroSleeping",
  "BroPunching",
  "Buildings",
  "MainMenuImpact",
  "Red",
  "Any key",
  "PlayButton",
  "OptionsButton",
  "CreditsButton",
  "ExitButton",
  "BackSky",
}

local OPTIONS_ENTITY_NAMES = {
  "OptionsPanel",
  MAIN_MENU_OPTIONS_CAT_NAME,
  "OptionsBGMLabel",
  "OptionsSFXLabel",
  "OptionsScreenLabel",
  "OptionsBGMBar",
  "OptionsSFXBar",
  "OptionsWindowed",
  "OptionsFullscreen",
  "OptionsBGMLeft",
  "OptionsBGMRight",
  "OptionsSFXLeft",
  "OptionsSFXRight",
  "OptionsScreenLeft",
  "OptionsScreenRight",
}

local BUTTON_TEXTURES = {
  PlayButton = {
    normal = "images/NewMainMenu/Buttons/PlayButton.png",
    hovered = "images/NewMainMenu/Buttons/PlayButtonHovered.PNG",
  },
  OptionsButton = {
    normal = "images/NewMainMenu/Buttons/OptionsButton.png",
    hovered = "images/NewMainMenu/Buttons/OptionsButtonHovered.PNG",
  },
  CreditsButton = {
    normal = "images/NewMainMenu/Buttons/CreditsButton.png",
    hovered = "images/NewMainMenu/Buttons/CreditsButtonHovered.PNG",
  },
  ExitButton = {
    normal = "images/NewMainMenu/Buttons/ExitButton.png",
    hovered = "images/NewMainMenu/Buttons/ExitButtonHovered.PNG",
  },
}

local BUTTON_TARGETS = {
  PlayButton = { x = 665.83, y = 294.58, delay = 0.00 },
  OptionsButton = { x = 989.17, y = 346.67, delay = 0.07 },
  CreditsButton = { x = 1054.58, y = 542.50, delay = 0.14 },
  ExitButton = { x = 925.42, y = 754.58, delay = 0.21 },
}

local BUTTON_START_POSITIONS = {
  PlayButton = { x = 4736.5908, y = -2802.7388 },
  OptionsButton = { x = 7005.0000, y = -1755.4059 },
  CreditsButton = { x = 8713.1445, y = 1169.2300 },
  ExitButton = { x = 9109.5029, y = 4277.5044 },
}

local OPTIONS_TARGETS = {
  OptionsPanel = { x = 933.2921, y = 394.4612, delay = 0.00 },
  MainMenuOptionsCat = { x = 483.2921, y = 579.4612, delay = 0.02 },
  OptionsBGMLabel = { x = 645.8967, y = 256.8189, delay = 0.03 },
  OptionsBGMBar = { x = 877.0935, y = 303.8838, delay = 0.05 },
  OptionsBGMLeft = { x = 626.0000, y = 303.8838, delay = 0.07 },
  OptionsBGMRight = { x = 1130.0000, y = 303.8838, delay = 0.09 },
  OptionsSFXLabel = { x = 639.9020, y = 357.9290, delay = 0.11 },
  OptionsSFXBar = { x = 877.3156, y = 413.0756, delay = 0.13 },
  OptionsSFXLeft = { x = 626.0000, y = 413.0756, delay = 0.15 },
  OptionsSFXRight = { x = 1130.0000, y = 413.0756, delay = 0.17 },
  OptionsScreenLabel = { x = 681.9982, y = 471.0284, delay = 0.19 },
  OptionsScreenLeft = { x = 626.0000, y = 528.9769, delay = 0.20 },
  OptionsWindowed = { x = 748.9164, y = 528.9769, delay = 0.21 },
  OptionsFullscreen = { x = 1001.4919, y = 528.9769, delay = 0.23 },
  OptionsScreenRight = { x = 1130.0000, y = 528.9769, delay = 0.24 },
}

local SLIDE_DURATION = 0.25
local OPTIONS_SLIDE_DURATION = 0.28
local OPTIONS_HIDDEN_OFFSET_X = 1800.0

local VOLUME_CHANGE_DEBOUNCE = 0.16
local last_bgm_change_time = 0.0
local last_sfx_change_time = 0.0

local function clamp(value, min_value, max_value)
  return math.min(math.max(value, min_value), max_value)
end

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

local function ease_out_cubic(t)
  local u = 1 - t
  return 1 - u * u * u
end

local function get_button_target(entity)
  if not entity then
    return nil
  end
  return BUTTON_TARGETS[entity:get_name()]
end

local function get_button_start_position(entity)
  if not entity then
    return nil
  end
  return BUTTON_START_POSITIONS[entity:get_name()]
end

local function get_named_entity(name)
  return ECS.get_entity_by_name(name)
end

local function get_options_cat_prefab()
  if options_cat_prefab == nil then
    options_cat_prefab = Resources.load_prefab(OPTIONS_CAT_PREFAB_PATH)
  end
  return options_cat_prefab
end

local function ensure_main_menu_options_cat()
  local cat = get_named_entity(MAIN_MENU_OPTIONS_CAT_NAME)
  if cat and cat:is_valid() then
    return cat
  end

  cat = ECS.instantiate_prefab(get_options_cat_prefab())
  if not cat or not cat:is_valid() then
    return nil
  end

  cat:set_name(MAIN_MENU_OPTIONS_CAT_NAME)
  cat:clear_layers()
  cat:add_to_layer("default")
  cat:add_to_layer("overlay_items_1")
  return cat
end

local function wrap_index(index, count)
  if count <= 0 then
    return 1
  end
  if index < 1 then
    return count
  end
  if index > count then
    return 1
  end
  return index
end

local function is_confirm_pressed()
  return Input.is_pressed(Key.Enter) or Input.is_pressed(Key.Space)
end

local function should_show_selection_hover()
  return not Input.was_last_input_mouse()
end

local function set_entity_alpha(entity_name, alpha)
  local entity = get_named_entity(entity_name)
  if not entity or not entity:is_valid() then
    return
  end

  local color = entity.Color
  if color then
    color.a = alpha
  end
end

local function get_option_target(entity_name)
  return OPTIONS_TARGETS[entity_name]
end

local function get_option_hidden_position(entity_name)
  local target = get_option_target(entity_name)
  if not target then
    return nil
  end

  return {
    x = target.x - OPTIONS_HIDDEN_OFFSET_X,
    y = target.y,
  }
end

local function is_options_controller(entity)
  return entity and entity:get_name() == OPTIONS_CONTROLLER_NAME
end

local function set_button_texture(entity_name, path)
  local entity = get_named_entity(entity_name)
  if not entity or not entity:is_valid() then
    return
  end

  local tex = entity.Texture
  if tex then
    tex.path = path
  end
end

local function reset_menu_button_textures()
  for entity_name, textures in pairs(BUTTON_TEXTURES) do
    set_button_texture(entity_name, textures.normal)
  end
end

local function get_active_menu_button_index()
  return hovered_menu_button_index or selected_menu_button_index
end

local function sync_main_menu_button_visuals()
  reset_menu_button_textures()

  if hovered_menu_button_index == nil and not should_show_selection_hover() then
    return
  end

  local entity_name = MENU_BUTTON_NAMES[get_active_menu_button_index()]
  local textures = entity_name and BUTTON_TEXTURES[entity_name]
  if textures then
    set_button_texture(entity_name, textures.hovered)
  end
end

local function set_menu_button_hover_index(index)
  hovered_menu_button_index = index
  if index ~= nil then
    selected_menu_button_index = index
  end
  sync_main_menu_button_visuals()
end

local function clear_menu_button_hover_index(index)
  if index ~= nil and hovered_menu_button_index ~= index then
    return
  end

  hovered_menu_button_index = nil
  sync_main_menu_button_visuals()
end

local function set_menu_button_selection(index)
  hovered_menu_button_index = nil
  selected_menu_button_index = wrap_index(index, #MENU_BUTTON_NAMES)
  sync_main_menu_button_visuals()
end

local function sync_option_focus_visuals()
  local show_focus = should_show_selection_hover()
  for row_index, entity_names in ipairs(OPTION_ROW_ENTITIES) do
    local alpha = 1.0
    if show_focus then
      alpha = row_index == selected_option_row_index and OPTION_ROW_FOCUSED_ALPHA or OPTION_ROW_UNFOCUSED_ALPHA
    end
    for _, entity_name in ipairs(entity_names) do
      set_entity_alpha(entity_name, alpha)
    end
  end
end

local function set_option_row_selection(index)
  selected_option_row_index = wrap_index(index, #OPTION_ROW_ENTITIES)
  sync_option_focus_visuals()
end

local function change_main_menu_state(state_name)
  for _, entity_name in ipairs(MAIN_MENU_ENTITY_NAMES) do
    local entity = get_named_entity(entity_name)
    if entity and entity:is_valid() then
      local sm = entity.StateMachine
      if sm and sm:has_state(state_name) then
        sm:change_state(state_name)
      end
    end
  end
end

local function set_entity_position(entity_name, x, y)
  local entity = get_named_entity(entity_name)
  if not entity or not entity:is_valid() then
    return
  end

  local tr = entity.Transform2D
  if tr then
    tr.position.x = x
    tr.position.y = y
  end
end

local function clear_menu_animations()
  for entity_name, _ in pairs(menu_anim) do
    menu_anim[entity_name] = nil
  end
end

local function queue_menu_animation(entity_name, target_x, target_y, delay, duration)
  local entity = get_named_entity(entity_name)
  if not entity or not entity:is_valid() then
    return
  end

  local tr = entity.Transform2D
  if not tr then
    return
  end

  menu_anim[entity_name] = {
    entity = entity,
    start_x = tr.position.x,
    start_y = tr.position.y,
    target_x = target_x,
    target_y = target_y,
    start_time = Time.get_time(),
    delay = delay or 0.0,
    duration = duration or SLIDE_DURATION,
  }
end

local function update_menu_animations()
  local now = Time.get_time()
  for entity_name, anim in pairs(menu_anim) do
    local entity = anim.entity
    if not entity or not entity:is_valid() then
      menu_anim[entity_name] = nil
    else
      local elapsed = now - anim.start_time - anim.delay
      if elapsed >= 0.0 then
        local t = math.min(elapsed / anim.duration, 1.0)
        local eased = ease_out_cubic(t)
        local tr = entity.Transform2D
        if tr then
          tr.position.x = anim.start_x + (anim.target_x - anim.start_x) * eased
          tr.position.y = anim.start_y + (anim.target_y - anim.start_y) * eased
        end

        if t >= 1.0 then
          menu_anim[entity_name] = nil
        end
      end
    end
  end
end

local function has_menu_animations()
  return next(menu_anim) ~= nil
end

local function snap_options_hidden()
  for _, entity_name in ipairs(OPTIONS_ENTITY_NAMES) do
    local hidden = get_option_hidden_position(entity_name)
    if hidden then
      set_entity_position(entity_name, hidden.x, hidden.y)
    end
  end
end

local function queue_options_in()
  for _, entity_name in ipairs(OPTIONS_ENTITY_NAMES) do
    local target = get_option_target(entity_name)
    if target then
      queue_menu_animation(entity_name, target.x, target.y, target.delay, OPTIONS_SLIDE_DURATION)
    end
  end
end

local function queue_options_out()
  for _, entity_name in ipairs(OPTIONS_ENTITY_NAMES) do
    local hidden = get_option_hidden_position(entity_name)
    if hidden then
      queue_menu_animation(entity_name, hidden.x, hidden.y, 0.0, OPTIONS_SLIDE_DURATION)
    end
  end
end

local function queue_buttons_to_menu()
  for entity_name, target in pairs(BUTTON_TARGETS) do
    queue_menu_animation(entity_name, target.x, target.y, target.delay, SLIDE_DURATION)
  end
end

local function queue_buttons_to_start()
  for entity_name, target in pairs(BUTTON_START_POSITIONS) do
    local button_target = BUTTON_TARGETS[entity_name]
    local delay = 0.0
    if button_target then
      delay = button_target.delay
    end
    queue_menu_animation(entity_name, target.x, target.y, delay, SLIDE_DURATION)
  end
end

local function menu_is_ready()
  local controller = get_named_entity(OPTIONS_CONTROLLER_NAME)
  if not controller or not controller:is_valid() then
    return false
  end

  if next(button_anim) ~= nil or has_menu_animations() then
    return false
  end

  local sm = controller.StateMachine
  return sm and (sm:is_in_state("Menu") or sm:is_in_state("Options"))
end

local function get_slider_texture_path(step)
  return string.format("images/NewMainMenu/Options/%d.png", clamp(step, 0, 10))
end

local function sync_options_visuals()
  local bgm_bar = get_named_entity("OptionsBGMBar")
  if bgm_bar and bgm_bar:is_valid() and bgm_bar.Texture then
    local bgm_step = clamp(math.floor(Settings.get_bgm_volume() * 10.0 + 0.5), 0, 10)
    bgm_bar.Texture.path = get_slider_texture_path(bgm_step)
  end

  local sfx_bar = get_named_entity("OptionsSFXBar")
  if sfx_bar and sfx_bar:is_valid() and sfx_bar.Texture then
    local sfx_step = clamp(math.floor(Settings.get_sfx_volume() * 10.0 + 0.5), 0, 10)
    sfx_bar.Texture.path = get_slider_texture_path(sfx_step)
  end

  local is_fullscreen = Settings.get_fullscreen()

  local windowed = get_named_entity("OptionsWindowed")
  if windowed and windowed:is_valid() and windowed.Texture then
    windowed.Texture.path = is_fullscreen and
      "images/NewMainMenu/Options/WindowedUnselected.png" or
      "images/NewMainMenu/Options/Windowed.png"
  end

  local fullscreen = get_named_entity("OptionsFullscreen")
  if fullscreen and fullscreen:is_valid() and fullscreen.Texture then
    fullscreen.Texture.path = is_fullscreen and
      "images/NewMainMenu/Options/Fullscreen.png" or
      "images/NewMainMenu/Options/FullscreenUnselected.png"
  end

  sync_option_focus_visuals()
end

local function open_options_menu()
  if options_active or not menu_is_ready() then
    return
  end

  options_active = true
  options_transition = "opening"
  hovered_menu_button_index = nil
  set_option_row_selection(1)
  reset_menu_button_textures()
  queue_buttons_to_start()
  queue_options_in()
  sync_options_visuals()
  change_main_menu_state("Options")
end

local function close_options_menu()
  if not options_active or options_transition == "closing" then
    return
  end

  options_active = false
  options_transition = "closing"
  reset_menu_button_textures()
  queue_buttons_to_menu()
  queue_options_out()
  change_main_menu_state("Menu")
end

function start_game()
  if options_active or options_transition ~= nil then
    return
  end
  Scene.load("OpeningCutsceneState")
end

function show_credits()
  if options_active or options_transition ~= nil then
    return
  end
  Scene.load("CreditsCutsceneState")
end

function options_menu()
  if not menu_is_ready() then
    return
  end

  if options_active then
    close_options_menu()
  else
    open_options_menu()
  end
end

function exit_game()
  if options_active or options_transition ~= nil then
    return
  end
  App.stop()
end

local function activate_selected_menu_button()
  local entity_name = MENU_BUTTON_NAMES[selected_menu_button_index]
  if entity_name == "PlayButton" then
    start_game()
  elseif entity_name == "OptionsButton" then
    options_menu()
  elseif entity_name == "ExitButton" then
    exit_game()
  end
end

local function change_bgm_volume(delta)
  local now = Time.get_time()
  if now - last_bgm_change_time < VOLUME_CHANGE_DEBOUNCE then
    return
  end
  last_bgm_change_time = now
  local volume = clamp(Settings.get_bgm_volume() + delta, 0.0, 1.0)
  Settings.set_bgm_volume(volume)
  sync_options_visuals()
end

local function change_sfx_volume(delta)
  local now = Time.get_time()
  if now - last_sfx_change_time < VOLUME_CHANGE_DEBOUNCE then
    return
  end
  last_sfx_change_time = now
  local volume = clamp(Settings.get_sfx_volume() + delta, 0.0, 1.0)
  Settings.set_sfx_volume(volume)
  sync_options_visuals()
end

local function adjust_selected_option(delta)
  if selected_option_row_index == 1 then
    change_bgm_volume(delta * 0.1)
  elseif selected_option_row_index == 2 then
    change_sfx_volume(delta * 0.1)
  elseif delta < 0 then
    main_menu_set_windowed()
  elseif delta > 0 then
    main_menu_set_fullscreen()
  end
end

function main_menu_bgm_decrease()
  if not options_active then
    return
  end
  set_option_row_selection(1)
  change_bgm_volume(-0.1)
end

function main_menu_bgm_increase()
  if not options_active then
    return
  end
  set_option_row_selection(1)
  change_bgm_volume(0.1)
end

local function handle_bar_click(entity_name, is_bgm)
  local entity = get_named_entity(entity_name)
  if not entity or not entity:is_valid() then
    return
  end
  local tr = entity.Transform2D
  if not tr then
    return
  end
  
  local mouse = Input.get_mouse_world_position()
  local width = 440.83334
  local left_edge = tr.position.x - (width * 0.5)
  local offset = mouse.x - left_edge
  local ratio = clamp(offset / width, 0.0, 1.0)
  
  local step = math.ceil(ratio * 10.0)
  step = clamp(step, 1, 10)
  local volume = step / 10.0
  
  if is_bgm then
    Settings.set_bgm_volume(volume)
  else
    Settings.set_sfx_volume(volume)
  end
  sync_options_visuals()
end

function main_menu_bgm_bar_click()
  if not options_active then return end
  set_option_row_selection(1)
  handle_bar_click("OptionsBGMBar", true)
end

function main_menu_sfx_bar_click()
  if not options_active then return end
  set_option_row_selection(2)
  handle_bar_click("OptionsSFXBar", false)
end

function main_menu_sfx_decrease()
  if not options_active then
    return
  end
  set_option_row_selection(2)
  change_sfx_volume(-0.1)
end

function main_menu_sfx_increase()
  if not options_active then
    return
  end
  set_option_row_selection(2)
  change_sfx_volume(0.1)
end

function main_menu_set_windowed()
  if not options_active then
    return
  end
  set_option_row_selection(3)
  Settings.set_fullscreen(false)
  sync_options_visuals()
end

function main_menu_set_fullscreen()
  if not options_active then
    return
  end
  set_option_row_selection(3)
  Settings.set_fullscreen(true)
  sync_options_visuals()
end

function main_menu_start_on_enter(entity)
  if is_options_controller(entity) then
    ensure_main_menu_options_cat()
    options_active = false
    options_transition = nil
    hovered_menu_button_index = nil
    selected_menu_button_index = 1
    selected_option_row_index = 1
    clear_menu_animations()
    snap_options_hidden()
    reset_menu_button_textures()
    sync_main_menu_button_visuals()
    sync_option_focus_visuals()
  end
end

function main_menu_start_on_update(entity)
  if options_active then
    return
  end

  if any_key_pressed() then
    local sm = entity.StateMachine
    if sm then
      sm:change_state("Transition")
    end
  end
end

function main_menu_start_on_exit(entity)
  Audio.stop_event(SPLASH_SFX_SNORE_EVENT, true)
  Audio.stop_event(SPLASH_SFX_TRAIN_EVENT, true)
  Audio.play_scene_music(MAIN_MENU_BGM_EVENT, 0.3)
end

function main_menu_bro_menu_enter(entity)
end

function main_menu_front_menu_enter(entity)
end

function main_menu_transition_on_enter(entity)
  if not entity:has("Clickable") then
    return
  end

  local tr = entity.Transform2D
  local target = get_button_target(entity)
  if not tr or not target then
    return
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
      return
    end

    local t = math.min(elapsed / anim.duration, 1.0)
    local eased = ease_out_cubic(t)
    local tr = entity.Transform2D
    if tr then
      tr.position.x = anim.start_x + (anim.target_x - anim.start_x) * eased
      tr.position.y = anim.start_y + (anim.target_y - anim.start_y) * eased
    end

    if t >= 1.0 then
      button_anim[eid] = nil
      if not next(button_anim) then
        if sm then
          sm:change_state("Menu")
        end
      end
    end
    return
  end

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

function main_menu_menu_on_enter(entity)
  if entity:has("Clickable") then
    local tr = entity.Transform2D
    local target = get_button_target(entity)
    if tr and target and options_transition ~= "closing" then
      tr.position.x = target.x
      tr.position.y = target.y
    end
  end

  if is_options_controller(entity) and options_transition ~= "closing" then
    snap_options_hidden()
    sync_main_menu_button_visuals()
  end
end

function main_menu_menu_on_update(entity)
  if not is_options_controller(entity) then
    return
  end

  update_menu_animations()
  sync_main_menu_button_visuals()

  if options_transition == "closing" and not has_menu_animations() then
    options_transition = nil
  end

  if not menu_is_ready() then
    return
  end

  if Input.is_pressed(Key.W) or Input.is_pressed(Key.A) then
    set_menu_button_selection(selected_menu_button_index - 1)
  end

  if Input.is_pressed(Key.S) or Input.is_pressed(Key.D) then
    set_menu_button_selection(selected_menu_button_index + 1)
  end

  if is_confirm_pressed() then
    activate_selected_menu_button()
  end
end

function main_menu_menu_on_exit(entity)
end

function main_menu_options_on_enter(entity)
  if entity:has("Clickable") then
    local tr = entity.Transform2D
    local target = get_button_start_position(entity)
    if tr and target and options_transition ~= "opening" then
      tr.position.x = target.x
      tr.position.y = target.y
    end
  end

  if is_options_controller(entity) then
    sync_options_visuals()
    set_option_row_selection(selected_option_row_index)
  end
end

function main_menu_options_on_update(entity)
  if not is_options_controller(entity) then
    return
  end

  update_menu_animations()
  sync_options_visuals()

  if options_transition == "opening" and not has_menu_animations() then
    options_transition = nil
  end

  if Input.is_pressed(Key.Escape) and not has_menu_animations() then
    close_options_menu()
    return
  end

  if has_menu_animations() then
    return
  end

  if Input.is_pressed(Key.W) then
    set_option_row_selection(selected_option_row_index - 1)
  end

  if Input.is_pressed(Key.S) then
    set_option_row_selection(selected_option_row_index + 1)
  end

  if Input.is_pressed(Key.A) then
    adjust_selected_option(-1)
  end

  if Input.is_pressed(Key.D) then
    adjust_selected_option(1)
  end
end

function main_menu_options_on_exit(entity)
end

function main_menu_play_hover(entity)
  if options_active then
    return
  end
  set_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.PlayButton)
end

function main_menu_play_exit(entity)
  clear_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.PlayButton)
end

function main_menu_options_hover(entity)
  if options_active then
    return
  end
  set_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.OptionsButton)
end

function main_menu_options_exit(entity)
  clear_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.OptionsButton)
end

function main_menu_credits_hover(entity)
  if options_active then
    return
  end
  set_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.CreditsButton)
end

function main_menu_credits_exit(entity)
  clear_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.CreditsButton)
end

function main_menu_exit_hover(entity)
  if options_active then
    return
  end
  set_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.ExitButton)
end

function main_menu_exit_exit(entity)
  clear_menu_button_hover_index(MENU_BUTTON_INDEX_BY_NAME.ExitButton)
end

register_blueprint("MainMenu", {
  initial_state = "Start",
  states = {
    { name = "Start",      on_enter = "main_menu_start_on_enter",      on_update = "main_menu_start_on_update",      on_exit = "main_menu_start_on_exit" },
    { name = "Transition", on_enter = "main_menu_transition_on_enter", on_update = "main_menu_transition_on_update", on_exit = "main_menu_transition_on_exit" },
    { name = "Menu",       on_enter = "main_menu_menu_on_enter",       on_update = "main_menu_menu_on_update",       on_exit = "main_menu_menu_on_exit" },
    { name = "Options",    on_enter = "main_menu_options_on_enter",    on_update = "main_menu_options_on_update",    on_exit = "main_menu_options_on_exit" },
  }
})