-- Constants for entity sizes
local PLAYER_SIZE = 20.0
local GROUND_WIDTH = 800.0
local GROUND_HEIGHT = 50.0
local WALL_WIDTH = 50.0
local WALL_HEIGHT = 300.0

function create_physics_entities()
    -- Create free falling player box
    local player = Game.create_entity()
    player:add_transform(800.0, 300.0, 0.0, PLAYER_SIZE, PLAYER_SIZE)
    player:add_color(1.0, 1.0, 1.0, 1.0)
    player:add_texture_quad()
    player:add_rigidbody(false, true)  -- not kinematic, use gravity
    player:add_box_collider(PLAYER_SIZE, PLAYER_SIZE, true)  -- width, height, is_trigger
    player:add_tag("BehaviourState")

    -- Create static ground
    local ground = Game.create_entity()
    ground:add_transform(800.0, 650.0, 0.0, GROUND_WIDTH, GROUND_HEIGHT)
    ground:add_color(0.2, 0.8, 0.2, 1.0)
    ground:add_texture_quad()
    ground:add_rigidbody(true, false)  -- kinematic, no gravity
    ground:add_box_collider(GROUND_WIDTH, GROUND_HEIGHT, false)
    ground:add_tag("BehaviourState")

    -- Create wall
    local wall = Game.create_entity()
    wall:add_transform(1200.0, 550.0, 0.0, WALL_WIDTH, WALL_HEIGHT)
    wall:add_color(0.2, 0.8, 0.2, 1.0)
    wall:add_texture_quad()
    wall:add_rigidbody(true, false)  -- kinematic, no gravity
    wall:add_box_collider(WALL_WIDTH, WALL_HEIGHT, false)
    wall:add_tag("BehaviourState")

    return player  -- Return player if you need to reference it later
end

-- ...existing code...

function free_resources()
    local entities = Game.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity:has_tag("BehaviourState") then
            Game.destroy_entity(entity.id)
        end
    end
end