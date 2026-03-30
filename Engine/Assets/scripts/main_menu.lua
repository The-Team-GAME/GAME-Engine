-- main_menu.lua
-- MainMenu state machine and button callbacks

local MAIN_MENU_BGM_EVENT    = "event:/BGM/MainMenu"
local SPLASH_SFX_SNORE_EVENT = "event:/SFX/SplashSnoreSFX"
local SPLASH_SFX_TRAIN_EVENT = "event:/SFX/SplashTrainSFX"

local OPTIONS_CONTROLLER_NAME = "Whole train"
local OPTIONS_PANEL_LAYER = "overlay_bg"
local OPTIONS_CONTENT_LAYER = "overlay_items_1"
local OPTIONS_CONTROL_LAYER = "overlay_items_3"

local button_anim = {}
local options_active = false
local options_ui = {}

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

local OPTIONS_LAYOUT = {
  panel = {
    name = "OptionsPanel",
    texture = "images/NewMainMenu/Options/New group.png",
    x = 933.2921,
    y = 394.4612,
    scale_x = 1171.6666,
    scale_y = 587.5,
    layer = OPTIONS_PANEL_LAYER,
  },
  bgm_label = {
    name = "OptionsBGMLabel",
    texture = "images/NewMainMenu/Options/BGM.png",
    x = 645.8967,
    y = 256.8189,
    scale_x = 97.5,
    scale_y = 40.0,
    layer = OPTIONS_CONTENT_LAYER,
  },
  sfx_label = {
    name = "OptionsSFXLabel",
    texture = "images/NewMainMenu/Options/SFX.png",
    x = 639.9020,
    y = 357.9290,
    scale_x = 85.8333,
    scale_y = 40.0,
    layer = OPTIONS_CONTENT_LAYER,
  },
  screen_label = {
    name = "OptionsScreenLabel",
    texture = "images/NewMainMenu/Options/SCREEN.png",
    x = 681.9982,
    y = 471.0284,
    scale_x = 175.8333,
    scale_y = 41.6667,
    layer = OPTIONS_CONTENT_LAYER,
  },
  bgm_bar = {
    name = "OptionsBGMBar",
    texture = "images/NewMainMenu/Options/10.png",
    x = 877.0935,
    y = 303.8838,
    scale_x = 440.8333,
    scale_y = 20.0,
    layer = OPTIONS_CONTENT_LAYER,
  },
  sfx_bar = {
    name = "OptionsSFXBar",
    texture = "images/NewMainMenu/Options/10.png",
    x = 877.3156,
    y = 413.0756,
    scale_x = 440.8333,
    scale_y = 20.0,
    layer = OPTIONS_CONTENT_LAYER,
  },
  windowed = {
    name = "OptionsWindowed",
    texture = "images/NewMainMenu/Options/Windowed.png",
    x = 748.9164,
    y = 528.9769,
    scale_x = 148.3333,
    scale_y = 26.6667,
    layer = OPTIONS_CONTENT_LAYER,
    click = { width = 148.3333, height = 26.6667, on_click = "main_menu_set_windowed" },
  },
  fullscreen = {
    name = "OptionsFullscreen",
    texture = "images/NewMainMenu/Options/FullscreenUnselected.png",
    x = 1001.4919,
    y = 528.9769,
    scale_x = 160.0,
    scale_y = 26.6667,
    layer = OPTIONS_CONTENT_LAYER,
    click = { width = 160.0, height = 26.6667, on_click = "main_menu_set_fullscreen" },
  },
  bgm_left = {
    name = "OptionsBGMLeft",
    texture = "images/NewMainMenu/Options/ArrowLeft.png",
    x = 700.0,
    y = 303.8838,
    scale_x = 42.0,
    scale_y = 42.0,
    layer = OPTIONS_CONTROL_LAYER,
    click = { width = 42.0, height = 42.0, on_click = "main_menu_bgm_decrease" },
  },
  bgm_right = {
    name = "OptionsBGMRight",
    texture = "images/NewMainMenu/Options/ArrowRight.png",
    x = 1054.0,
    y = 303.8838,
    scale_x = 42.0,
    scale_y = 42.0,
    layer = OPTIONS_CONTROL_LAYER,
    click = { width = 42.0, height = 42.0, on_click = "main_menu_bgm_increase" },
  },
  sfx_left = {
    name = "OptionsSFXLeft",
    texture = "images/NewMainMenu/Options/ArrowLeft.png",
    x = 700.0,
    y = 413.0756,
    scale_x = 42.0,
    scale_y = 42.0,
    layer = OPTIONS_CONTROL_LAYER,
    click = { width = 42.0, height = 42.0, on_click = "main_menu_sfx_decrease" },
  },
  sfx_right = {
    name = "OptionsSFXRight",
    texture = "images/NewMainMenu/Options/ArrowRight.png",
    x = 1054.0,
    y = 413.0756,
    scale_x = 42.0,
    scale_y = 42.0,
    layer = OPTIONS_CONTROL_LAYER,
    click = { width = 42.0, height = 42.0, on_click = "main_menu_sfx_increase" },
  },
}

local SLIDE_DURATION = 0.25

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

local function get_named_entity(name)
  return ECS.get_entity_by_name(name)
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

local function menu_is_ready()
  local controller = get_named_entity(OPTIONS_CONTROLLER_NAME)
  if not controller or not controller:is_valid() then
    return false
  end

  local sm = controller.StateMachine
  return sm and (sm:is_in_state("Menu") or sm:is_in_state("Options"))
end

local function destroy_options_ui()
  for key, entity in pairs(options_ui) do
    if entity and entity:is_valid() then
      ECS.destroy_entity(entity)
    end
    options_ui[key] = nil
  end
end

local function create_options_entity(def)
  local entity = ECS.create_entity()
  entity:set_name(def.name)
  entity:clear_layers()
  entity:add_to_layer(def.layer)
  entity:add("Transform2D")
  entity:add("Texture")
  entity:add("Color")

  local tr = entity.Transform2D
  tr.position.x = def.x
  tr.position.y = def.y
  tr.rotation = 0.0
  tr.scale.x = def.scale_x
  tr.scale.y = def.scale_y

  local tex = entity.Texture
  tex.path = def.texture

  local color = entity.Color
  color.r = 1.0
  color.g = 1.0
  color.b = 1.0
  color.a = 1.0

  if def.click then
    entity:add("BoxCollider")
    local collider = entity.BoxCollider
    collider.resolve_collisions = false
    collider.collided = false
    collider.type = ColliderType.AABB
    collider.width = def.click.width
    collider.height = def.click.height
    collider.aabb.half_width = def.click.width * 0.5
    collider.aabb.half_height = def.click.height * 0.5
    collider.offset.x = 0.0
    collider.offset.y = 0.0

    entity:add("Clickable")
    entity.Clickable.on_click.name = def.click.on_click or ""
    entity.Clickable.on_hover.name = def.click.on_hover or ""
    entity.Clickable.exit_hover.name = def.click.on_exit or ""
  end

  return entity
end

local function ensure_options_ui()
  if next(options_ui) ~= nil then
    return
  end

  for key, def in pairs(OPTIONS_LAYOUT) do
    options_ui[key] = create_options_entity(def)
  end
end

local function volume_to_step(volume)
  return clamp(math.floor(volume * 10.0 + 0.5), 1, 10)
end

local function step_to_volume(step)
  return clamp(step, 1, 10) / 10.0
end

local function get_slider_texture_path(step)
  return string.format("images/NewMainMenu/Options/%d.png", clamp(step, 1, 10))
end

local function sync_options_visuals()
  if not options_active or next(options_ui) == nil then
    return
  end

  local bgm_bar = options_ui.bgm_bar
  if bgm_bar and bgm_bar:is_valid() and bgm_bar.Texture then
    bgm_bar.Texture.path = get_slider_texture_path(volume_to_step(Settings.get_bgm_volume()))
  end

  local sfx_bar = options_ui.sfx_bar
  if sfx_bar and sfx_bar:is_valid() and sfx_bar.Texture then
    sfx_bar.Texture.path = get_slider_texture_path(volume_to_step(Settings.get_sfx_volume()))
  end

  local is_fullscreen = Settings.get_fullscreen()
  local windowed = options_ui.windowed
  if windowed and windowed:is_valid() and windowed.Texture then
    windowed.Texture.path = is_fullscreen and
      "images/NewMainMenu/Options/WindowedUnselected.png" or
      "images/NewMainMenu/Options/Windowed.png"
  end

  local fullscreen = options_ui.fullscreen
  if fullscreen and fullscreen:is_valid() and fullscreen.Texture then
    fullscreen.Texture.path = is_fullscreen and
      "images/NewMainMenu/Options/Fullscreen.png" or
      "images/NewMainMenu/Options/FullscreenUnselected.png"
  end
end

local function open_options_menu()
  if options_active or not menu_is_ready() then
    return
  end

  options_active = true
  reset_menu_button_textures()
  change_main_menu_state("Options")
end

local function close_options_menu()
  if not options_active then
    return
  end

  options_active = false
  reset_menu_button_textures()
  destroy_options_ui()
  change_main_menu_state("Menu")
end

function start_game()
  if options_active then
    return
  end
  Scene.load("OpeningCutsceneState")
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
  if options_active then
    return
  end
  App.stop()
end

function main_menu_bgm_decrease()
  if not options_active then
    return
  end
  local step = volume_to_step(Settings.get_bgm_volume()) - 1
  Settings.set_bgm_volume(step_to_volume(step))
  sync_options_visuals()
end

function main_menu_bgm_increase()
  if not options_active then
    return
  end
  local step = volume_to_step(Settings.get_bgm_volume()) + 1
  Settings.set_bgm_volume(step_to_volume(step))
  sync_options_visuals()
end

function main_menu_sfx_decrease()
  if not options_active then
    return
  end
  local step = volume_to_step(Settings.get_sfx_volume()) - 1
  Settings.set_sfx_volume(step_to_volume(step))
  sync_options_visuals()
end

function main_menu_sfx_increase()
  if not options_active then
    return
  end
  local step = volume_to_step(Settings.get_sfx_volume()) + 1
  Settings.set_sfx_volume(step_to_volume(step))
  sync_options_visuals()
end

function main_menu_set_windowed()
  if not options_active then
    return
  end
  Settings.set_fullscreen(false)
  sync_options_visuals()
end

function main_menu_set_fullscreen()
  if not options_active then
    return
  end
  Settings.set_fullscreen(true)
  sync_options_visuals()
end

function main_menu_start_on_enter(entity)
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
    if tr and target then
      tr.position.x = target.x
      tr.position.y = target.y
    end
  end

  if is_options_controller(entity) then
    destroy_options_ui()
  end
end

function main_menu_menu_on_update(entity)
end

function main_menu_menu_on_exit(entity)
end

function main_menu_options_on_enter(entity)
  if entity:has("Clickable") then
    local tr = entity.Transform2D
    local target = get_button_target(entity)
    if tr and target then
      tr.position.x = target.x
      tr.position.y = target.y
    end
  end

  if is_options_controller(entity) then
    ensure_options_ui()
    sync_options_visuals()
  end
end

function main_menu_options_on_update(entity)
  if not is_options_controller(entity) then
    return
  end

  if Input.is_pressed(Key.Escape) then
    close_options_menu()
    return
  end

  sync_options_visuals()
end

function main_menu_options_on_exit(entity)
  if is_options_controller(entity) then
    destroy_options_ui()
  end
end

function main_menu_play_hover(entity)
  if options_active then
    return
  end
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.PlayButton.hovered
  end
end

function main_menu_play_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.PlayButton.normal
  end
end

function main_menu_options_hover(entity)
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.OptionsButton.hovered
  end
end

function main_menu_options_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.OptionsButton.normal
  end
end

function main_menu_credits_hover(entity)
  if options_active then
    return
  end
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.CreditsButton.hovered
  end
end

function main_menu_credits_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.CreditsButton.normal
  end
end

function main_menu_exit_hover(entity)
  if options_active then
    return
  end
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.ExitButton.hovered
  end
end

function main_menu_exit_exit(entity)
  local tex = entity.Texture
  if tex then
    tex.path = BUTTON_TEXTURES.ExitButton.normal
  end
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
