-- This is an example of Scripting using .lua files
-- using mainmenustate as an example

-- Store button globally so we can access it in update
local start_button = nil
local transform = nil
local move_speed = 300.0  -- pixels per second

local quad = nil
local quad_transform = nil

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

function create_quad()
    local temp = Game.create_entity()
    temp:add_transform(350.0, 700.0, 0.0, 100.0, 100.0)
    temp:add_texture("images/monkey.png")
    temp:add_color(1.0, 1.0, 1.0, 1.0)
    temp:add_tag("MainMenuState")

    quad = temp  -- Store reference
    quad_transform = temp:get_component("Transform2D")

    return temp;
end

function create_start_button()
    local btn = Game.create_entity()
    btn:add_transform(800.0, 700.0, 0.0, 100.0, 100.0)
    btn:add_texture("images/banana.png")
    btn:add_color(1.0, 1.0, 1.0, 1.0)
    btn:add_tag("MainMenuState")

    start_button = btn  -- Store reference
    
    --Set the trasform here

    transform = btn:get_component("Transform2D")

    return btn
end

function chase_button(dt)
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

function create_menu_entities()
    -- create_menu_background()
    create_quad()
    create_start_button()
    log_message("Menu entities created from Lua!")
end