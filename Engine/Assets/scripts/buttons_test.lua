-- This is an example of Scripting using .lua files
-- using mainmenustate as an example

-- Store button globally so we can access it in update
local start_button = nil
local transform = nil
local move_speed = 300.0  -- pixels per second

local quad = nil
local quad_transform = nil

local PLAYER_WIDTH = 20.0
local PLAYER_HEIGHT = 12.0
local PLAYER_SCALE = 3.0

local CHASER_WIDTH = 60.0
local CHASER_HEIGHT = 84.0
local CHASER_SCALE = 2.0

-- I am going to locally store the translate of the button

-- function create_menu_background()
--     return Game.create_from_table{
--         components = {
--             { _type = "Transform", x = 800.0, y = 450.0, rotation = 0.0, sx = 1600.0, sy = 900.0 },
--             { _type = "Texture", path = "images/UI/MAIN MENU.png" },
--             { _type = "Color", r = 1.0, g = 1.0, b = 1.0, a = 1.0 },
--             { _type = "Tag", tag = "MainMenuState"}
--         }
--     }
-- end

function create_chaser()
    local temp = Game.create_entity()
    temp:add_transform(350.0, 700.0, 0.0, 
        CHASER_WIDTH * CHASER_SCALE, 
        CHASER_HEIGHT * CHASER_SCALE)
    temp:add_texture("images/monkey.png")
    temp:add_color(1.0, 1.0, 1.0, 1.0)
    temp:add_tag("LuaState")

    quad = temp  -- Store reference
    quad_transform = temp:get_component("Transform2D")

    return temp;
end

function create_player()
    local player = Game.create_entity()
    player:add_transform(800.0, 700.0, 0.0, 
        PLAYER_WIDTH * PLAYER_SCALE,
        PLAYER_HEIGHT * PLAYER_SCALE)
    player:add_texture("images/banana.png")
    player:add_color(1.0, 1.0, 1.0, 1.0)
    player:add_tag("LuaState")

    start_button = player  -- Store reference

    --Set the trasform here

    transform = player:get_component("Transform2D")

    return btn
end

function free_resources()
    -- Get all entities and destroy them
    local entities = Game.get_all_entities()
    for _, entity in ipairs(entities) do
        Game.destroy_entity(entity.id)
    end
    
    -- Clear our global references
    start_button = nil
    quad = nil
    menu_background = nil
end

function chase(dt)
    if not quad or not start_button then return end

    local q = quad:get_component("Transform2D")
    local s = start_button:get_component("Transform2D")
    if not q or not s then return end

    local dx = s.x - q.x
    local dy = s.y - q.y
    local dist = math.sqrt(dx * dx + dy * dy)
    if dist <= 1.0 then return end

    local chase_speed = 200.0 -- do not chase too fast
    local nx = dx / dist
    local ny = dy / dist

    q.x = q.x + nx * chase_speed * dt
    q.y = q.y + ny * chase_speed * dt

    -- Face target???
    q.rotation = math.atan2(ny, nx)
end
-- Add update function to handle movement
function update_movement(dt)
    if not start_button then return end
    
    -- Get transform component directly
    local transform = start_button:get_component("Transform2D")
    if transform then
        if Game.is_key_held("W") then
            transform.y = transform.y - move_speed * dt
        end
        if Game.is_key_held("S") then
            transform.y = transform.y + move_speed * dt
        end
        if Game.is_key_held("A") then
            transform.x = transform.x - move_speed * dt
        end
        if Game.is_key_held("D") then
            transform.x = transform.x + move_speed * dt
        end
    end
end

function create_test()
    -- create_menu_background()
    create_chaser()
    create_player()
    log_message("Menu entities created from Lua!")
end