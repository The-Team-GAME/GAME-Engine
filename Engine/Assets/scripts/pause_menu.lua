local PLAYER_HEARTBEAT_EVENT = "event:/SFX/PlayerHeartBeat"
local PAUSE_DIMMER_NAME = "PauseMenuDimmer"
local PAUSE_CONTROLLER_NAME = "PauseMenuController"
local LEFT_PANEL_NAME = "PauseLeftPanel"
local OPTIONS_PANEL_NAME = "PauseOptionsPanel"
local OPTIONS_CAT_NAME = "PauseOptionsCat"
local OPTIONS_CAT_FRAME_DURATION = 0.08
local OPTIONS_CAT_TOTAL_FRAMES = 26
local OPTIONS_CAT_LOOP_START_FRAME = 4
local OPTIONS_BGM_LABEL_NAME = "PauseOptionsBGMLabel"
local OPTIONS_SFX_LABEL_NAME = "PauseOptionsSFXLabel"
local OPTIONS_SCREEN_LABEL_NAME = "PauseOptionsScreenLabel"
local OPTIONS_BGM_BAR_NAME = "PauseOptionsBGMBar"
local OPTIONS_SFX_BAR_NAME = "PauseOptionsSFXBar"
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
local SCREEN_LEFT_BUTTON_NAME = "PauseScreenLeftButton"
local WINDOWED_BUTTON_NAME = "PauseWindowedButton"
local FULLSCREEN_BUTTON_NAME = "PauseFullscreenButton"
local SCREEN_RIGHT_BUTTON_NAME = "PauseScreenRightButton"
local CONFIRM_YES_BUTTON_NAME = "PauseConfirmYesButton"
local CONFIRM_NO_BUTTON_NAME = "PauseConfirmNoButton"

local MENU_SCROLL_NAMES = { "PausePausedScrollA", "PausePausedScrollB" }
local RESTART_YES_SCROLL_NAMES = { "PauseRestartYesScrollA", "PauseRestartYesScrollB" }
local RESTART_NO_SCROLL_NAMES = { "PauseRestartNoScrollA", "PauseRestartNoScrollB" }

local HOVER_LAYER_NAME = "overlay_items_hover"
local FULLSCREEN_CENTER_X = 800.0
local FULLSCREEN_CENTER_Y = 450.0
local SCREEN_HEIGHT = 900.0
local SCROLL_HEIGHT = 1800.0
local SCROLL_SPEED = 90.0
local VOLUME_MAX_STEP = 10
local LEFT_CLUSTER_HIDDEN_MARGIN = 40.0
local LEFT_CLUSTER_SLIDE_DURATION = 0.32

local LEFT_PANEL_TEXTURES = {
  menu = "images/UI/In-Game Pause/States/NothingSelectedPausedMenu.png",
  resume = "images/UI/In-Game Pause/States/ResumeButton.png",
  restart = "images/UI/In-Game Pause/States/RestartButton.png",
  options = "images/UI/In-Game Pause/States/OptionsButtons.png",
  quit = "images/UI/In-Game Pause/States/ExitButton.png",
}

local OPTIONS_TEXTURES = {
  panel = "images/NewMainMenu/Options/New group.png",
  bgm_label = "images/NewMainMenu/Options/BGM.png",
  sfx_label = "images/NewMainMenu/Options/SFX.png",
  screen_label = "images/NewMainMenu/Options/SCREEN.png",
  bar_template = "images/NewMainMenu/Options/%d.png",
  arrow_left = "images/NewMainMenu/Options/ArrowLeft.png",
  arrow_right = "images/NewMainMenu/Options/ArrowRight.png",
  windowed = "images/NewMainMenu/Options/Windowed.png",
  windowed_unselected = "images/NewMainMenu/Options/WindowedUnselected.png",
  fullscreen = "images/NewMainMenu/Options/Fullscreen.png",
  fullscreen_unselected = "images/NewMainMenu/Options/FullscreenUnselected.png",
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

local OPTIONS_CAT_PREFAB_PATH = "prefabs/cat_options_idle.json"

local RIGHT_PANEL_NAMES = {
  OPTIONS_PANEL_NAME,
  OPTIONS_CAT_NAME,
  OPTIONS_BGM_LABEL_NAME,
  OPTIONS_SFX_LABEL_NAME,
  OPTIONS_SCREEN_LABEL_NAME,
  OPTIONS_BGM_BAR_NAME,
  OPTIONS_SFX_BAR_NAME,
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

local LEFT_CLUSTER_ENTITY_NAMES = {
  LEFT_PANEL_NAME,
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
  SCREEN_LEFT_BUTTON_NAME,
  WINDOWED_BUTTON_NAME,
  FULLSCREEN_BUTTON_NAME,
  SCREEN_RIGHT_BUTTON_NAME,
}

local CONFIRM_BUTTON_NAMES = {
  CONFIRM_YES_BUTTON_NAME,
  CONFIRM_NO_BUTTON_NAME,
}

local ENTITY_LAYER_NAMES = {
  [PAUSE_DIMMER_NAME] = "overlay_bg",
  [LEFT_PANEL_NAME] = "overlay_items_2",
  [OPTIONS_PANEL_NAME] = "overlay_items_1",
  [OPTIONS_CAT_NAME] = "overlay_items_2",
  [OPTIONS_BGM_LABEL_NAME] = "overlay_items_2",
  [OPTIONS_SFX_LABEL_NAME] = "overlay_items_2",
  [OPTIONS_SCREEN_LABEL_NAME] = "overlay_items_2",
  [OPTIONS_BGM_BAR_NAME] = "overlay_items_2",
  [OPTIONS_SFX_BAR_NAME] = "overlay_items_2",
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
  [SCREEN_LEFT_BUTTON_NAME] = "overlay_items_3",
  [WINDOWED_BUTTON_NAME] = "overlay_items_3",
  [FULLSCREEN_BUTTON_NAME] = "overlay_items_3",
  [SCREEN_RIGHT_BUTTON_NAME] = "overlay_items_3",
  [CONFIRM_YES_BUTTON_NAME] = "overlay_items_3",
  [CONFIRM_NO_BUTTON_NAME] = "overlay_items_3",
}

local ENTITY_VISIBLE_ALPHA = {
  [PAUSE_DIMMER_NAME] = 0.5,
}

local LEFT_HOVER_TARGETS = {
  { key = "resume", entity_name = RESUME_BUTTON_NAME },
  { key = "restart", entity_name = RESTART_BUTTON_NAME },
  { key = "options", entity_name = OPTIONS_BUTTON_NAME },
  { key = "quit", entity_name = EXIT_BUTTON_NAME },
}

local LEFT_HOVER_KEY_BY_ENTITY_NAME = {
  [RESUME_BUTTON_NAME] = "resume",
  [RESTART_BUTTON_NAME] = "restart",
  [OPTIONS_BUTTON_NAME] = "options",
  [EXIT_BUTTON_NAME] = "quit",
}

local CONFIRM_HOVER_TARGETS = {
  { key = "yes", entity_name = CONFIRM_YES_BUTTON_NAME },
  { key = "no", entity_name = CONFIRM_NO_BUTTON_NAME },
}

local CONFIRM_HOVER_KEY_BY_ENTITY_NAME = {
  [CONFIRM_YES_BUTTON_NAME] = "yes",
  [CONFIRM_NO_BUTTON_NAME] = "no",
}

local OPTION_ROW_ENTITIES = {
  { OPTIONS_BGM_LABEL_NAME, OPTIONS_BGM_BAR_NAME, BGM_DECREASE_BUTTON_NAME, BGM_INCREASE_BUTTON_NAME },
  { OPTIONS_SFX_LABEL_NAME, OPTIONS_SFX_BAR_NAME, SFX_DECREASE_BUTTON_NAME, SFX_INCREASE_BUTTON_NAME },
  { OPTIONS_SCREEN_LABEL_NAME, SCREEN_LEFT_BUTTON_NAME, WINDOWED_BUTTON_NAME, FULLSCREEN_BUTTON_NAME, SCREEN_RIGHT_BUTTON_NAME },
}

local OPTION_ROW_FOCUSED_ALPHA = 1.0
local OPTION_ROW_UNFOCUSED_ALPHA = 0.72

local hovered_left_button = nil
local selected_left_button = LEFT_HOVER_TARGETS[1].key
local hovered_confirm_button = nil
local selected_confirm_button = CONFIRM_HOVER_TARGETS[1].key
local pause_options_selected_index = 1
local left_cluster_base_positions = {}
local left_cluster_base_bounds = nil
local left_cluster_anim = {}
local left_cluster_has_opened = false
local options_textures_warmed = false
local options_cat_prefab = nil
local options_cat_animation_active = false
local options_cat_anim_started_at = 0.0
local options_cat_intro_played = false

local function clamp(value, min_value, max_value)
  return math.min(math.max(value, min_value), max_value)
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

local function get_target_index(entries, key)
  for index, entry in ipairs(entries) do
    if entry.key == key then
      return index
    end
  end
  return 1
end

local function is_confirm_pressed()
  return Input.is_pressed(Key.Enter) or Input.is_pressed(Key.Space)
end

local function should_show_selection_hover()
  return not Input.was_last_input_mouse()
end

local function ease_out_cubic(t)
  local u = 1.0 - t
  return 1.0 - u * u * u
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

local function set_pause_options_cat_frame(frame_index)
  local cat = get_named_entity(OPTIONS_CAT_NAME)
  if not cat or not cat:is_valid() then
    return
  end

  local atlas = cat.TextureAtlas
  if atlas then
    atlas.drawIndex = clamp(frame_index, 0, OPTIONS_CAT_TOTAL_FRAMES - 1)
  end
end

local function start_pause_options_cat_animation()
  options_cat_animation_active = true
  options_cat_anim_started_at = Time.get_unscaled_time()

  if options_cat_intro_played then
    set_pause_options_cat_frame(OPTIONS_CAT_LOOP_START_FRAME)
  else
    set_pause_options_cat_frame(0)
  end
end

local function stop_pause_options_cat_animation()
  options_cat_animation_active = false
end

local function update_pause_options_cat_animation()
  if not options_cat_animation_active then
    return
  end

  local cat = get_named_entity(OPTIONS_CAT_NAME)
  if not cat or not cat:is_valid() then
    return
  end

  local elapsed = Time.get_unscaled_time() - options_cat_anim_started_at

  if not options_cat_intro_played then
    local intro_frame = math.floor(elapsed / OPTIONS_CAT_FRAME_DURATION)
    if intro_frame < OPTIONS_CAT_LOOP_START_FRAME then
      set_pause_options_cat_frame(intro_frame)
      return
    end

    options_cat_intro_played = true
    options_cat_anim_started_at = Time.get_unscaled_time()
    elapsed = 0.0
  end

  local loop_frame_count = OPTIONS_CAT_TOTAL_FRAMES - OPTIONS_CAT_LOOP_START_FRAME
  local loop_frame = math.floor(elapsed / OPTIONS_CAT_FRAME_DURATION) % loop_frame_count
  set_pause_options_cat_frame(OPTIONS_CAT_LOOP_START_FRAME + loop_frame)
end

local function ensure_pause_options_cat()
  local cat = get_named_entity(OPTIONS_CAT_NAME)
  if cat and cat:is_valid() then
    if cat:has("Animation") then
      cat:remove("Animation")
    end

    local tr = cat.Transform2D
    if tr then
      tr.position.x = 128.0
      tr.position.y = 715.0
      tr.scale.x = 400.0
      tr.scale.y = 400.0
    end

    return cat
  end

  cat = ECS.instantiate_prefab(get_options_cat_prefab())
  if not cat or not cat:is_valid() then
    return nil
  end

  cat:set_name(OPTIONS_CAT_NAME)

  if cat:has("Animation") then
    cat:remove("Animation")
  end

  if not cat:has("UI") then
    cat:add("UI")
  end

  local ui = cat.UI
  if ui then
    ui.anchor.x = 0.0
    ui.anchor.y = 0.0
  end

  local tr = cat.Transform2D
  if tr then
    tr.position.x = 120.0
    tr.position.y = 785.0
    tr.scale.x = 400.0
    tr.scale.y = 400.0
  end

  cat:clear_layers()
  cat:add_to_layer(ENTITY_LAYER_NAMES[OPTIONS_CAT_NAME] or "overlay_items_2")
  return cat
end

local function get_pause_controller()
  return get_named_entity(PAUSE_CONTROLLER_NAME)
end

local function is_pause_controller(entity)
  return entity and entity:is_valid() and entity:get_name() == PAUSE_CONTROLLER_NAME
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

local function set_entity_layer(name, layer_name)
  local entity = get_named_entity(name)
  if not entity or not entity:is_valid() then
    return
  end

  entity:clear_layers()
  entity:add_to_layer(layer_name)
end

local function restore_entity_base_layer(name)
  local layer_name = ENTITY_LAYER_NAMES[name]
  if layer_name ~= nil then
    set_entity_layer(name, layer_name)
  end
end

local function raise_entity_to_hover_layer(name)
  set_entity_layer(name, HOVER_LAYER_NAME)
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

local function set_entity_position(name, x, y)
  local entity = get_named_entity(name)
  if not entity or not entity:is_valid() then
    return
  end

  local tr = entity.Transform2D
  if tr then
    tr.position.x = x
    tr.position.y = y
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

local function cache_left_cluster_layout()
  if left_cluster_base_bounds ~= nil then
    return
  end

  local min_top = nil
  local max_bottom = nil

  for _, entity_name in ipairs(LEFT_CLUSTER_ENTITY_NAMES) do
    local entity = get_named_entity(entity_name)
    if entity and entity:is_valid() then
      local tr = entity.Transform2D
      if tr then
        left_cluster_base_positions[entity_name] = {
          x = tr.position.x,
          y = tr.position.y,
        }
      end
    end
  end

  for _, entity_name in ipairs(LEFT_CLUSTER_ENTITY_NAMES) do
    local entity = get_named_entity(entity_name)
    local base = left_cluster_base_positions[entity_name]
    if entity and entity:is_valid() and base ~= nil then
      local tr = entity.Transform2D
      local box = entity.BoxCollider
      local top = nil
      local bottom = nil
      if box then
        top = base.y + box.offset.y - box.height * 0.5
        bottom = base.y + box.offset.y + box.height * 0.5
      elseif tr then
        top = base.y - tr.scale.y * 0.5
        bottom = base.y + tr.scale.y * 0.5
      end
      if top ~= nil and bottom ~= nil then
        if min_top == nil or top < min_top then
          min_top = top
        end
        if max_bottom == nil or bottom > max_bottom then
          max_bottom = bottom
        end
      end
    end
  end

  left_cluster_base_bounds = {
    top = min_top or 0.0,
    bottom = max_bottom or SCREEN_HEIGHT,
  }
end

local function get_left_cluster_target_offset(mode)
  cache_left_cluster_layout()
  local bounds = left_cluster_base_bounds
  if bounds == nil then
    return 0.0
  end

  if mode == "hidden" then
    return -LEFT_CLUSTER_HIDDEN_MARGIN - bounds.bottom
  end

  return 0.0
end

local function set_left_cluster_mode(mode, instant)
  local offset = get_left_cluster_target_offset(mode)

  if instant then
    for _, entity_name in ipairs(LEFT_CLUSTER_ENTITY_NAMES) do
      local base = left_cluster_base_positions[entity_name]
      if base ~= nil then
        set_entity_position(entity_name, base.x, base.y + offset)
      end
      left_cluster_anim[entity_name] = nil
    end
    return
  end

  local now = Time.get_unscaled_time()
  for _, entity_name in ipairs(LEFT_CLUSTER_ENTITY_NAMES) do
    local base = left_cluster_base_positions[entity_name]
    local entity = get_named_entity(entity_name)
    if base ~= nil and entity and entity:is_valid() then
      local tr = entity.Transform2D
      if tr then
        left_cluster_anim[entity_name] = {
          start_x = tr.position.x,
          start_y = tr.position.y,
          target_x = base.x,
          target_y = base.y + offset,
          start_time = now,
          duration = LEFT_CLUSTER_SLIDE_DURATION,
        }
      end
    end
  end
end

local function show_left_cluster_at_menu_position()
  if not left_cluster_has_opened then
    set_left_cluster_mode("hidden", true)
    left_cluster_has_opened = true
  end

  set_left_cluster_mode("menu", false)
end

local function hide_left_cluster()
  left_cluster_has_opened = true
  set_left_cluster_mode("hidden", false)
end

local function update_left_cluster_animation()
  local now = Time.get_unscaled_time()

  for entity_name, anim in pairs(left_cluster_anim) do
    local entity = get_named_entity(entity_name)
    if not entity or not entity:is_valid() then
      left_cluster_anim[entity_name] = nil
    else
      local tr = entity.Transform2D
      if not tr then
        left_cluster_anim[entity_name] = nil
      else
        local t = clamp((now - anim.start_time) / anim.duration, 0.0, 1.0)
        local eased = ease_out_cubic(t)
        tr.position.x = anim.start_x + (anim.target_x - anim.start_x) * eased
        tr.position.y = anim.start_y + (anim.target_y - anim.start_y) * eased

        if t >= 1.0 then
          left_cluster_anim[entity_name] = nil
        end
      end
    end
  end
end

local function get_mouse_screen_position()
  local mouse = Input.get_mouse_position_if_in_viewport()
  if not mouse then
    return nil
  end

  return mouse
end

local function point_in_polygon(point, vertices)
  local inside = false
  local count = #vertices
  local j = count

  for i = 1, count do
    local vi = vertices[i]
    local vj = vertices[j]
    local intersects = ((vi.y > point.y) ~= (vj.y > point.y))
      and (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x)

    if intersects then
      inside = not inside
    end

    j = i
  end

  return inside
end

local function get_button_polygon_vertices(tr, box)
  local polygon = box.polygon
  if not polygon or not polygon.vertices or #polygon.vertices < 3 then
    return nil
  end

  local center_x = tr.position.x + box.offset.x
  local center_y = tr.position.y + box.offset.y
  local vertices = {}

  for _, vertex in ipairs(polygon.vertices) do
    vertices[#vertices + 1] = {
      x = center_x + vertex.x,
      y = center_y + vertex.y,
    }
  end

  return vertices
end

local function is_mouse_over_button(entity_name, mouse)
  local entity = get_named_entity(entity_name)
  if not entity or not entity:is_valid() then
    return false
  end

  local tr = entity.Transform2D
  local box = entity.BoxCollider
  if not tr or not box then
    return false
  end

  local polygon_vertices = get_button_polygon_vertices(tr, box)
  if polygon_vertices ~= nil then
    return point_in_polygon(mouse, polygon_vertices)
  end

  local half_width = box.width * 0.5
  local half_height = box.height * 0.5
  local center_x = tr.position.x + box.offset.x
  local center_y = tr.position.y + box.offset.y

  return mouse.x >= center_x - half_width and mouse.x <= center_x + half_width
    and mouse.y >= center_y - half_height and mouse.y <= center_y + half_height
end

local function get_hover_target(entries)
  local mouse = get_mouse_screen_position()
  if mouse == nil then
    return nil
  end

  for _, entry in ipairs(entries) do
    if is_mouse_over_button(entry.entity_name, mouse) then
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

local function get_active_left_button()
  return hovered_left_button or selected_left_button
end

local function refresh_left_panel()
  local path = get_default_left_panel_texture()
  if hovered_left_button == nil and not should_show_selection_hover() then
    restore_entity_base_layer(LEFT_PANEL_NAME)
    set_entity_texture(LEFT_PANEL_NAME, path)
    return
  end

  local active_button = get_active_left_button()
  if active_button ~= nil then
    path = LEFT_PANEL_TEXTURES[active_button] or path
    raise_entity_to_hover_layer(LEFT_PANEL_NAME)
  else
    restore_entity_base_layer(LEFT_PANEL_NAME)
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
  return clamp(math.floor(volume * VOLUME_MAX_STEP + 0.5), 0, VOLUME_MAX_STEP)
end

local function step_to_volume(step)
  return clamp(step / VOLUME_MAX_STEP, 0.0, 1.0)
end

local function get_volume_bar_path(step)
  return string.format(OPTIONS_TEXTURES.bar_template, clamp(step, 0, VOLUME_MAX_STEP))
end

local function warm_options_texture_variants()
  if options_textures_warmed then
    return
  end

  local paths = {
    OPTIONS_TEXTURES.panel,
    OPTIONS_TEXTURES.bgm_label,
    OPTIONS_TEXTURES.sfx_label,
    OPTIONS_TEXTURES.screen_label,
    OPTIONS_TEXTURES.arrow_left,
    OPTIONS_TEXTURES.arrow_right,
    OPTIONS_TEXTURES.windowed,
    OPTIONS_TEXTURES.windowed_unselected,
    OPTIONS_TEXTURES.fullscreen,
    OPTIONS_TEXTURES.fullscreen_unselected,
  }

  for step = 0, VOLUME_MAX_STEP do
    paths[#paths + 1] = get_volume_bar_path(step)
  end

  for _, path in ipairs(paths) do
    Resources.load_texture(path)
  end

  options_textures_warmed = true
end

local sync_option_focus_visuals

local function sync_options_visuals()
  set_entity_texture(OPTIONS_BGM_BAR_NAME, get_volume_bar_path(volume_to_step(Settings.get_bgm_volume())))
  set_entity_texture(OPTIONS_SFX_BAR_NAME, get_volume_bar_path(volume_to_step(Settings.get_sfx_volume())))

  local is_fullscreen = Settings.get_fullscreen()
  set_entity_texture(WINDOWED_BUTTON_NAME, is_fullscreen and OPTIONS_TEXTURES.windowed_unselected or OPTIONS_TEXTURES.windowed)
  set_entity_texture(FULLSCREEN_BUTTON_NAME, is_fullscreen and OPTIONS_TEXTURES.fullscreen or OPTIONS_TEXTURES.fullscreen_unselected)

  sync_option_focus_visuals()
end

sync_option_focus_visuals = function()
  local show_focus = should_show_selection_hover()
  for row_index, entity_names in ipairs(OPTION_ROW_ENTITIES) do
    local alpha = 1.0
    if show_focus then
      alpha = row_index == pause_options_selected_index and OPTION_ROW_FOCUSED_ALPHA or OPTION_ROW_UNFOCUSED_ALPHA
    end
    for _, entity_name in ipairs(entity_names) do
      set_entity_alpha(entity_name, alpha)
    end
  end
end

local function set_option_selection(index)
  pause_options_selected_index = wrap_index(index, #OPTION_ROW_ENTITIES)
  sync_option_focus_visuals()
end

local function hide_options_hover_overlay()
  return
end

local function get_confirm_texture_set()
  return {
    yes_default = CONFIRM_TEXTURES.quit_yes_default,
    yes_hover = CONFIRM_TEXTURES.quit_yes_hover,
    no_default = CONFIRM_TEXTURES.quit_no_default,
    no_hover = CONFIRM_TEXTURES.quit_no_hover,
  }
end

local function get_active_confirm_choice()
  return hovered_confirm_button or selected_confirm_button
end

local function reset_confirm_quit_visuals()
  local textures = get_confirm_texture_set()
  set_entity_texture(CONFIRM_QUIT_YES_NAME, textures.yes_default)
  set_entity_texture(CONFIRM_QUIT_NO_NAME, textures.no_default)
  restore_entity_base_layer(CONFIRM_QUIT_YES_NAME)
  restore_entity_base_layer(CONFIRM_QUIT_NO_NAME)
end

local function apply_confirm_visual(choice)
  if hovered_confirm_button == nil and not should_show_selection_hover() then
    reset_confirm_quit_visuals()
    return
  end

  local textures = get_confirm_texture_set()

  if choice == "yes" then
      set_entity_texture(CONFIRM_QUIT_YES_NAME, textures.yes_hover)
      set_entity_texture(CONFIRM_QUIT_NO_NAME, textures.no_default)
      raise_entity_to_hover_layer(CONFIRM_QUIT_YES_NAME)
      restore_entity_base_layer(CONFIRM_QUIT_NO_NAME)
  elseif choice == "no" then
      set_entity_texture(CONFIRM_QUIT_YES_NAME, textures.yes_default)
      set_entity_texture(CONFIRM_QUIT_NO_NAME, textures.no_hover)
      restore_entity_base_layer(CONFIRM_QUIT_YES_NAME)
      raise_entity_to_hover_layer(CONFIRM_QUIT_NO_NAME)
  else
    reset_confirm_quit_visuals()
  end
end

local function apply_confirm_hover(choice)
  hovered_confirm_button = choice
  if choice ~= nil then
    selected_confirm_button = choice
  end

  apply_confirm_visual(get_active_confirm_choice())
end

local function set_confirm_selection(choice)
  hovered_confirm_button = nil
  selected_confirm_button = choice
  apply_confirm_visual(selected_confirm_button)
end

local function update_left_hover_from_mouse()
  return
end

local function update_options_hover_from_mouse()
  return
end

local function update_confirm_hover_from_mouse()
  return
end

local function show_options_panel()
  ensure_pause_options_cat()
  set_entity_visible(OPTIONS_CAT_NAME, true)
  set_entity_visible(OPTIONS_PANEL_NAME, true)
  set_entity_visible(OPTIONS_BGM_LABEL_NAME, true)
  set_entity_visible(OPTIONS_SFX_LABEL_NAME, true)
  set_entity_visible(OPTIONS_SCREEN_LABEL_NAME, true)
  set_entity_visible(OPTIONS_BGM_BAR_NAME, true)
  set_entity_visible(OPTIONS_SFX_BAR_NAME, true)
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
  show_confirm_quit_panel()
end

local function show_menu_panel()
  set_entities_visible(MENU_SCROLL_NAMES, true)
  update_scroll_pair(MENU_SCROLL_NAMES)
end

local function set_left_hover(button_name)
  hovered_left_button = button_name
  if button_name ~= nil then
    selected_left_button = button_name
  end
  refresh_left_panel()
end

local function clear_left_hover()
  set_left_hover(nil)
end

local function set_left_selection(button_name)
  hovered_left_button = nil
  if button_name ~= nil then
    selected_left_button = button_name
  end
  refresh_left_panel()
end

local function move_left_selection(delta)
  local current_index = get_target_index(LEFT_HOVER_TARGETS, selected_left_button)
  local next_index = wrap_index(current_index + delta, #LEFT_HOVER_TARGETS)
  set_left_selection(LEFT_HOVER_TARGETS[next_index].key)
end

local function move_confirm_selection(delta)
  local current_index = get_target_index(CONFIRM_HOVER_TARGETS, selected_confirm_button)
  local next_index = wrap_index(current_index + delta, #CONFIRM_HOVER_TARGETS)
  set_confirm_selection(CONFIRM_HOVER_TARGETS[next_index].key)
end

local function change_bgm_step(delta)
  if not is_pause_state("Options") then
    return
  end

  local step = volume_to_step(Settings.get_bgm_volume()) + delta
  Settings.set_bgm_volume(step_to_volume(clamp(step, 0, VOLUME_MAX_STEP)))
  sync_options_visuals()
end

local function change_sfx_step(delta)
  if not is_pause_state("Options") then
    return
  end

  local step = volume_to_step(Settings.get_sfx_volume()) + delta
  Settings.set_sfx_volume(step_to_volume(clamp(step, 0, VOLUME_MAX_STEP)))
  sync_options_visuals()
end

local function handle_bar_click(entity_name, is_bgm)
  if not is_pause_state("Options") then
    return
  end

  local entity = get_named_entity(entity_name)
  if not entity or not entity:is_valid() then
    return
  end

  local tr = entity.Transform2D
  local box = entity.BoxCollider
  local mouse = get_mouse_screen_position()
  if not tr or not box or mouse == nil then
    return
  end

  local left_edge = tr.position.x + box.offset.x - box.width * 0.5
  local ratio = clamp((mouse.x - left_edge) / box.width, 0.0, 1.0)
  local step = math.ceil(ratio * VOLUME_MAX_STEP)
  step = clamp(step, 1, VOLUME_MAX_STEP)

  if is_bgm then
    Settings.set_bgm_volume(step / VOLUME_MAX_STEP)
  else
    Settings.set_sfx_volume(step / VOLUME_MAX_STEP)
  end

  sync_options_visuals()
end

function pause_menu_resume()
  set_left_selection("resume")
  Scene.pop()
end

function pause_menu_restart()
  set_left_selection("restart")
  if not is_pause_state("ConfirmRestart") then
    change_pause_state("ConfirmRestart")
  end
end

function pause_menu_options()
  set_left_selection("options")
  if is_pause_state("Options") then
    change_pause_state("Menu")
  else
    change_pause_state("Options")
  end
end

function pause_menu_exit()
  set_left_selection("quit")
  if not is_pause_state("ConfirmQuit") then
    change_pause_state("ConfirmQuit")
  end
end

function pause_bgm_decrease()
  set_option_selection(1)
  change_bgm_step(-1)
end

function pause_bgm_increase()
  set_option_selection(1)
  change_bgm_step(1)
end

function pause_bgm_bar_click()
  set_option_selection(1)
  handle_bar_click(OPTIONS_BGM_BAR_NAME, true)
end

function pause_sfx_decrease()
  set_option_selection(2)
  change_sfx_step(-1)
end

function pause_sfx_increase()
  set_option_selection(2)
  change_sfx_step(1)
end

function pause_sfx_bar_click()
  set_option_selection(2)
  handle_bar_click(OPTIONS_SFX_BAR_NAME, false)
end

function pause_set_windowed()
  if not is_pause_state("Options") then
    return
  end

  set_option_selection(3)
  Settings.set_fullscreen(false)
  sync_options_visuals()
end

function pause_set_fullscreen()
  if not is_pause_state("Options") then
    return
  end

  set_option_selection(3)
  Settings.set_fullscreen(true)
  sync_options_visuals()
end

local function adjust_selected_option(delta)
  if pause_options_selected_index == 1 then
    change_bgm_step(delta)
  elseif pause_options_selected_index == 2 then
    change_sfx_step(delta)
  elseif delta < 0 then
    pause_set_windowed()
  elseif delta > 0 then
    pause_set_fullscreen()
  end
end

function pause_confirm_yes()
  set_confirm_selection("yes")
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
  set_confirm_selection("no")
  if is_pause_state("ConfirmRestart") or is_pause_state("ConfirmQuit") then
    change_pause_state("Menu")
  end
end

local function activate_selected_left_button()
  local active_button = get_active_left_button()
  if active_button == "resume" then
    pause_menu_resume()
  elseif active_button == "restart" then
    pause_menu_restart()
  elseif active_button == "options" then
    pause_menu_options()
  elseif active_button == "quit" then
    pause_menu_exit()
  end
end

local function activate_selected_confirm_button()
  local choice = get_active_confirm_choice()
  if choice == "yes" then
    pause_confirm_yes()
  else
    pause_confirm_no()
  end
end

function pause_hover_resume(entity)
  set_left_hover("resume")
end

function pause_hover_restart(entity)
  set_left_hover("restart")
end

function pause_hover_options(entity)
  set_left_hover("options")
end

function pause_hover_exit(entity)
  set_left_hover("quit")
end

function pause_left_button_exit_hover(entity)
  if not entity or not entity:is_valid() then
    clear_left_hover()
    return
  end

  local hover_key = LEFT_HOVER_KEY_BY_ENTITY_NAME[entity:get_name()]
  if hover_key ~= nil and hovered_left_button == hover_key then
    clear_left_hover()
  end
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
  apply_confirm_hover("yes")
end

function pause_confirm_no_hover(entity)
  apply_confirm_hover("no")
end

function pause_confirm_hover_exit(entity)
  if not entity or not entity:is_valid() then
    apply_confirm_hover(nil)
    return
  end

  local hover_key = CONFIRM_HOVER_KEY_BY_ENTITY_NAME[entity:get_name()]
  if hover_key ~= nil and hovered_confirm_button == hover_key then
    apply_confirm_hover(nil)
  end
end

function pause_menu_menu_on_enter(entity)
  ensure_pause_options_cat()
  warm_options_texture_variants()
  hovered_left_button = nil
  hovered_confirm_button = nil
  show_left_cluster_at_menu_position()
  show_base_pause_entities()
  hide_right_panels()
  hide_state_specific_buttons()
  show_menu_panel()
  refresh_left_panel()
  update_left_hover_from_mouse()
end

function pause_menu_menu_on_update(entity)
  if not is_pause_controller(entity) then
    return
  end

  update_left_cluster_animation()
  update_scroll_pair(MENU_SCROLL_NAMES)
  update_left_hover_from_mouse()
  refresh_left_panel()

  if Input.is_pressed(Key.Escape) then
    pause_menu_resume()
    return
  end

  if Input.is_pressed(Key.W) or Input.is_pressed(Key.A) then
    move_left_selection(-1)
  end

  if Input.is_pressed(Key.S) or Input.is_pressed(Key.D) then
    move_left_selection(1)
  end

  if is_confirm_pressed() then
    activate_selected_left_button()
  end
end

function pause_menu_menu_on_exit(entity)
end

function pause_menu_options_on_enter(entity)
  ensure_pause_options_cat()
  start_pause_options_cat_animation()
  hovered_left_button = nil
  hovered_confirm_button = nil
  selected_left_button = "options"
  show_base_pause_entities()
  hide_left_cluster()
  hide_right_panels()
  hide_state_specific_buttons()
  set_entities_visible(OPTIONS_BUTTON_NAMES, true)
  set_option_selection(1)
  show_options_panel()
  refresh_left_panel()
  update_left_hover_from_mouse()
  update_options_hover_from_mouse()
end

function pause_menu_options_on_update(entity)
  if not is_pause_controller(entity) then
    return
  end

  update_left_cluster_animation()
  update_pause_options_cat_animation()
  update_left_hover_from_mouse()
  update_options_hover_from_mouse()
  refresh_left_panel()
  sync_options_visuals()

  if Input.is_pressed(Key.Escape) then
    change_pause_state("Menu")
    return
  end

  if Input.is_pressed(Key.W) then
    set_option_selection(pause_options_selected_index - 1)
  end

  if Input.is_pressed(Key.S) then
    set_option_selection(pause_options_selected_index + 1)
  end

  if Input.is_pressed(Key.A) then
    adjust_selected_option(-1)
  end

  if Input.is_pressed(Key.D) then
    adjust_selected_option(1)
  end
end

function pause_menu_options_on_exit(entity)
  stop_pause_options_cat_animation()
  hide_options_hover_overlay()
end

function pause_menu_confirm_restart_on_enter(entity)
  hovered_left_button = nil
  hovered_confirm_button = nil
  selected_left_button = "restart"
  show_base_pause_entities()
  hide_left_cluster()
  hide_right_panels()
  hide_state_specific_buttons()
  set_entities_visible(CONFIRM_BUTTON_NAMES, true)
  show_confirm_restart_panel()
  set_confirm_selection(CONFIRM_HOVER_TARGETS[1].key)
  refresh_left_panel()
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()
end

function pause_menu_confirm_restart_on_update(entity)
  if not is_pause_controller(entity) then
    return
  end

  update_left_cluster_animation()
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()
  refresh_left_panel()
  apply_confirm_visual(get_active_confirm_choice())

  if Input.is_pressed(Key.Escape) then
    change_pause_state("Menu")
    return
  end

  if Input.is_pressed(Key.W) or Input.is_pressed(Key.A) then
    move_confirm_selection(-1)
  end

  if Input.is_pressed(Key.S) or Input.is_pressed(Key.D) then
    move_confirm_selection(1)
  end

  if is_confirm_pressed() then
    activate_selected_confirm_button()
  end
end

function pause_menu_confirm_restart_on_exit(entity)
end

function pause_menu_confirm_quit_on_enter(entity)
  hovered_left_button = nil
  hovered_confirm_button = nil
  selected_left_button = "quit"
  show_base_pause_entities()
  hide_left_cluster()
  hide_right_panels()
  hide_state_specific_buttons()
  set_entities_visible(CONFIRM_BUTTON_NAMES, true)
  show_confirm_quit_panel()
  set_confirm_selection(CONFIRM_HOVER_TARGETS[1].key)
  refresh_left_panel()
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()
end

function pause_menu_confirm_quit_on_update(entity)
  if not is_pause_controller(entity) then
    return
  end

  update_left_cluster_animation()
  update_left_hover_from_mouse()
  update_confirm_hover_from_mouse()
  refresh_left_panel()
  apply_confirm_visual(get_active_confirm_choice())

  if Input.is_pressed(Key.Escape) then
    change_pause_state("Menu")
    return
  end

  if Input.is_pressed(Key.W) or Input.is_pressed(Key.A) then
    move_confirm_selection(-1)
  end

  if Input.is_pressed(Key.S) or Input.is_pressed(Key.D) then
    move_confirm_selection(1)
  end

  if is_confirm_pressed() then
    activate_selected_confirm_button()
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