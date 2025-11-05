-- Store button globally so we can access it in update
local start_button = nil
local transform = nil
local move_speed = 300.0  -- pixels per second

-- I am going to locally store the translate of the button

function create_menu_background()
    return Game.create_from_table{
        components = {
            { _type = "Transform", x = 800.0, y = 450.0, rotation = 0.0, sx = 1600.0, sy = 900.0 },
            { _type = "Texture", path = "images/UI/MAIN MENU.png" },
            { _type = "Color", r = 1.0, g = 1.0, b = 1.0, a = 1.0 },
            { _type = "Tag", tag = "MainMenuState"}
        }
    }
end

function create_start_button()
    local btn = Game.create_entity()
    btn:add_transform(800.0, 700.0, 0.0, 192.0 * 1.5, 64.0 * 1.5)
    btn:add_texture("images/UI/Button_Start.png")
    btn:add_color(1.0, 1.0, 1.0, 1.0)
    btn:add_tag("MainMenuState")

    start_button = btn  -- Store reference
    
    --Set the trasform here

    transform = start_button:get_transform()

    return btn
end

-- Add update function to handle movement
function update_menu(dt)
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
    create_menu_background()
    create_start_button()
    log_message("Menu entities created from Lua!")
end