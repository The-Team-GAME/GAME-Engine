-- ================================================================================
-- file:    PlayerStateCallbacks.lua
-- project: Monkey Stole My GPA
--
-- author:  How Hong Chin Ryan (Lua port)
--          Original C++ by Chew Shee Yang
--
-- brief:   Player state machine callback implementations in Lua
--
-- NOTE: This is a template/reference implementation. The attack functions that
--       create hitbox entities are complex and may be better kept in C++ due to
--       the need for dynamic component addition. For a working Lua implementation,
--       you would need to either:
--       1. Create C++ helper functions for attack hitbox creation
--       2. Implement comprehensive entity building API in Lua bindings
--       3. Keep attack states in C++ (PlayerStateCallbacks.h) while moving
--          simpler states (Idle, Walking, Jumping) to Lua
--
-- Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
-- ================================================================================

-- ================================================================================
-- Idle State Callbacks
-- ================================================================================

function player_idle_enter(entity)
    local rigid = entity:get_component("RigidBody")
    if rigid then
        rigid.velocity.x = 0.0
    end
    print("Player: Entered Idle state")
end

function player_idle_update(entity)
    local state_machine = entity:get_component("StateMachine")
    
    -- Check for horizontal movement input (A/D only for sidescrolling)
    if Input.is_held(KeyCode.A) or Input.is_held(KeyCode.D) then
        state_machine:change_state("Walking")
    end
    
    -- Check for jump input
    if Input.is_pressed(KeyCode.Space) then
        local rigid = entity:get_component("RigidBody")
        -- Only jump if on ground
        if rigid and rigid.on_ground then
            state_machine:change_state("Jumping")
        end
    end
    
    -- Check for attack input
    if Input.is_pressed(KeyCode.F) then
        state_machine:change_state("Attack1")
    end
end

-- ================================================================================
-- Walking State Callbacks
-- ================================================================================

function player_walking_enter(entity)
    print("Player: Entered Walking state")
end

function player_walking_update(entity)
    local rigid = entity:get_component("RigidBody")
    local state_machine = entity:get_component("StateMachine")
    local flip = entity:get_component("Flip")
    local player = entity:get_component("Player")
    
    if not player then return end
    
    rigid.velocity.x = 0.0
    -- Don't touch velocity.y - let gravity handle it
    
    -- Horizontal movement only (sidescrolling)
    if Input.is_held(KeyCode.A) then
        rigid.velocity.x = -player.walk_speed
        if flip then flip.value.x = -1.0 end -- Face left
    end
    if Input.is_held(KeyCode.D) then
        rigid.velocity.x = player.walk_speed
        if flip then flip.value.x = 1.0 end -- Face right
    end
    
    -- Return to idle if no horizontal input
    if not Input.is_held(KeyCode.A) and not Input.is_held(KeyCode.D) then
        state_machine:change_state("Idle")
    end
    
    -- Check for jump input
    if Input.is_pressed(KeyCode.Space) then
        -- Only jump if on ground
        if rigid and rigid.on_ground then
            state_machine:change_state("Jumping")
        end
    end
    
    -- Can attack while moving
    if Input.is_pressed(KeyCode.F) then
        state_machine:change_state("Attack1")
    end
end

function player_walking_exit(entity)
    local rigid = entity:get_component("RigidBody")
    if rigid then
        rigid.velocity.x = 0.0
        -- Don't reset velocity.y - let gravity/physics handle it
    end
end

-- ================================================================================
-- Jumping State Callbacks
-- ================================================================================

function player_jumping_enter(entity)
    local rigid = entity:get_component("RigidBody")
    local player = entity:get_component("Player")
    
    if rigid and player then
        -- Apply jump force
        rigid.velocity.y = -player.jump_force -- Negative is up
    end
    
    print("Player: Entered Jumping state")
end

function player_jumping_update(entity)
    local rigid = entity:get_component("RigidBody")
    local state_machine = entity:get_component("StateMachine")
    local flip = entity:get_component("Flip")
    local player = entity:get_component("Player")
    
    if not player then return end
    
    -- Allow horizontal air control
    rigid.velocity.x = 0.0
    
    if Input.is_held(KeyCode.A) then
        rigid.velocity.x = -player.air_speed
        if flip then flip.value.x = -1.0 end -- Face left
    end
    if Input.is_held(KeyCode.D) then
        rigid.velocity.x = player.air_speed
        if flip then flip.value.x = 1.0 end -- Face right
    end
    
    -- Return to idle when landing
    if rigid and rigid.on_ground then
        -- Check if moving horizontally
        if Input.is_held(KeyCode.A) or Input.is_held(KeyCode.D) then
            state_machine:change_state("Walking")
        else
            state_machine:change_state("Idle")
        end
    end
    
    -- Can attack while jumping
    if Input.is_pressed(KeyCode.F) then
        state_machine:change_state("Attack1")
    end
end

-- ================================================================================
-- Attack 1 State Callbacks (First Punch)
-- ================================================================================

function player_attack1_enter(entity)
    local rigid = entity:get_component("RigidBody")
    local flip = entity:get_component("Flip")
    local player = entity:get_component("Player")
    
    -- Apply forward movement during attack
    if rigid and flip and player then
        if flip.value.x < 0 then
            rigid.velocity.x = -player.attack1.dash_speed
        else
            rigid.velocity.x = player.attack1.dash_speed
        end
    end
    
    -- Create attack hitbox entity
    local transform = entity:get_component("Transform2D")
    
    if transform and player then
        local offset_x = player.attack1.hitbox_offset_x
        if flip and flip.value.x < 0 then
            offset_x = -offset_x
        end
        local offset_y = player.attack1.hitbox_offset_y
        
        local hitbox = ECS.create_entity()
        hitbox:add_component("Tag", "PlayerAttackHitbox")
        hitbox:add_component("Attack", player.attack1.damage)
        hitbox:add_component("Transform2D",
            transform.position.x + offset_x,
            transform.position.y + offset_y,
            0.0,
            player.attack1.hitbox_width,
            player.attack1.hitbox_height
        )
        hitbox:add_component("RigidBody", true, false)
        hitbox:add_component("BoxCollider", player.attack1.hitbox_width, player.attack1.hitbox_height, false)
        hitbox:add_component("Parent", entity)
        hitbox:add_component("Lifetime", 0.48) -- Attack1 duration: 6 frames × 0.08f
        hitbox:add_component("Texture", Graphics.get_rect_texture())
        hitbox:remove_from_layer("default")
        hitbox:add_to_layer("damage_enemy")
        
        -- Apply knockback direction based on facing
        local attack = hitbox:get_component("Attack")
        if attack then
            attack.knockback = Vec2D(player.attack1.knockback.x, player.attack1.knockback.y)
            if flip and flip.value.x < 0 then
                attack.knockback.x = -attack.knockback.x
            end
            -- Knockback Y is always positive (up) in our config, but physics needs negative for up
            attack.knockback.y = -math.abs(attack.knockback.y)
        end
    end
    
    print("Player: Attack 1 (First Punch)")
end

function player_attack1_update(entity)
    local state_machine = entity:get_component("StateMachine")
    local animation = entity:get_component("Animation")
    
    if not animation then return end
    
    -- Check if player pressed attack again during this combo window
    if Input.is_pressed(KeyCode.F) and not animation:has_ended() then
        -- Allow combo after frame 3
        if animation:get_current_frame_index() >= 3 then
            state_machine:change_state("Attack2")
        end
    end
    
    -- Transition to idle when attack animation completes
    if animation:has_ended() then
        local transform = entity:get_component("Transform2D")
        local flip = entity:get_component("Flip")
        -- offset slightly forward because attack texture is wider than idle texture
        if transform and flip then
            if flip.value.x < 0 then
                transform.position.x = transform.position.x + 20.0
            else
                transform.position.x = transform.position.x - 20.0
            end
        end
        
        -- Force so that we don't display the attack texture after offset is applied
        state_machine:change_state("Idle")
    end
end

-- ================================================================================
-- Attack 2 State Callbacks (Second Punch)
-- ================================================================================

function player_attack2_enter(entity)
    local rigid = entity:get_component("RigidBody")
    local flip = entity:get_component("Flip")
    local player = entity:get_component("Player")
    
    -- Apply forward movement during attack
    if rigid and flip and player then
        if flip.value.x < 0 then
            rigid.velocity.x = -player.attack2.dash_speed
        else
            rigid.velocity.x = player.attack2.dash_speed
        end
    end
    
    -- Create attack hitbox entity
    local transform = entity:get_component("Transform2D")
    
    if transform and player then
        local offset_x = player.attack2.hitbox_offset_x
        if flip and flip.value.x < 0 then
            offset_x = -offset_x
        end
        local offset_y = player.attack2.hitbox_offset_y
        
        local hitbox = ECS.create_entity()
        hitbox:add_component("Tag", "PlayerAttackHitbox")
        hitbox:add_component("Attack", player.attack2.damage)
        hitbox:add_component("Transform2D",
            transform.position.x + offset_x,
            transform.position.y + offset_y,
            0.0,
            player.attack2.hitbox_width,
            player.attack2.hitbox_height
        )
        hitbox:add_component("RigidBody", true, false)
        hitbox:add_component("BoxCollider", player.attack2.hitbox_width, player.attack2.hitbox_height, false)
        hitbox:add_component("Parent", entity)
        hitbox:add_component("Lifetime", 0.24) -- Attack2 duration: 3 frames × 0.08f
        hitbox:add_component("Texture", Graphics.get_rect_texture())
        hitbox:remove_from_layer("default")
        hitbox:add_to_layer("damage_enemy")
        
        -- Apply knockback direction based on facing
        local attack = hitbox:get_component("Attack")
        if attack then
            attack.knockback = Vec2D(player.attack2.knockback.x, player.attack2.knockback.y)
            if flip and flip.value.x < 0 then
                attack.knockback.x = -attack.knockback.x
            end
            -- Knockback Y is always positive (up) in our config, but physics needs negative for up
            attack.knockback.y = -math.abs(attack.knockback.y)
        end
    end
    
    print("Player: Attack 2 (Second Punch)")
end

function player_attack2_update(entity)
    local state_machine = entity:get_component("StateMachine")
    local animation = entity:get_component("Animation")
    
    if not animation then return end
    
    -- Check if player pressed attack again during this combo window
    if Input.is_pressed(KeyCode.F) and not animation:has_ended() then
        -- Allow combo after frame 1 of this attack
        if animation:get_current_frame_index() >= 1 then
            state_machine:change_state("Attack3")
        end
    end
    
    -- Transition to idle when attack animation completes
    if animation:has_ended() then
        local transform = entity:get_component("Transform2D")
        local flip = entity:get_component("Flip")
        -- offset slightly forward because attack texture is wider than idle texture
        if transform and flip then
            if flip.value.x < 0 then
                transform.position.x = transform.position.x + 20.0
            else
                transform.position.x = transform.position.x - 20.0
            end
        end
        
        -- Force so that we don't display the attack texture after offset is applied
        state_machine:change_state("Idle")
    end
end

-- ================================================================================
-- Attack 3 State Callbacks (Third Punch - Finisher)
-- ================================================================================

function player_attack3_enter(entity)
    local rigid = entity:get_component("RigidBody")
    local flip = entity:get_component("Flip")
    local player = entity:get_component("Player")
    
    -- Apply forward movement during attack
    if rigid and flip and player then
        if flip.value.x < 0 then
            rigid.velocity.x = -player.attack3.dash_speed
        else
            rigid.velocity.x = player.attack3.dash_speed
        end
    end
    
    -- Create attack hitbox entity
    local transform = entity:get_component("Transform2D")
    
    if transform and player then
        local offset_x = player.attack3.hitbox_offset_x
        if flip and flip.value.x < 0 then
            offset_x = -offset_x
        end
        local offset_y = player.attack3.hitbox_offset_y
        
        local hitbox = ECS.create_entity()
        hitbox:add_component("Tag", "PlayerAttackHitbox")
        hitbox:add_component("Attack", player.attack3.damage)
        hitbox:add_component("Transform2D",
            transform.position.x + offset_x,
            transform.position.y + offset_y,
            0.0,
            player.attack3.hitbox_width,
            player.attack3.hitbox_height
        )
        hitbox:add_component("RigidBody", true, false)
        hitbox:add_component("BoxCollider", player.attack3.hitbox_width, player.attack3.hitbox_height, false)
        hitbox:add_component("Parent", entity)
        hitbox:add_component("Lifetime", 0.6) -- Attack3 duration: 10 frames × 0.06f
        hitbox:add_component("Texture", Graphics.get_rect_texture())
        hitbox:remove_from_layer("default")
        hitbox:add_to_layer("damage_enemy")
        
        -- Apply knockback direction based on facing
        local attack = hitbox:get_component("Attack")
        if attack then
            attack.knockback = Vec2D(player.attack3.knockback.x, player.attack3.knockback.y)
            if flip and flip.value.x < 0 then
                attack.knockback.x = -attack.knockback.x
            end
            -- Knockback Y is always positive (up) in our config, but physics needs negative for up
            attack.knockback.y = -math.abs(attack.knockback.y)
        end
    end
    
    print("Player: Attack 3 (Third Punch - Finisher!)")
end

function player_attack3_update(entity)
    local state_machine = entity:get_component("StateMachine")
    local animation = entity:get_component("Animation")
    
    -- No combo after this - it's the finisher
    -- Just wait for animation to complete
    if animation and animation:has_ended() then
        local transform = entity:get_component("Transform2D")
        local flip = entity:get_component("Flip")
        -- offset slightly forward because attack texture is wider than idle texture
        if transform and flip then
            if flip.value.x < 0 then
                transform.position.x = transform.position.x + 20.0
            else
                transform.position.x = transform.position.x - 20.0
            end
        end
        
        -- Force so that we don't display the attack texture after offset is applied
        state_machine:change_state("Idle")
    end
end
