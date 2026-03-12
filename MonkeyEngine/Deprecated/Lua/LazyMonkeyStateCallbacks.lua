-----------------------------------------------------------------------------------
-- file:    LazyMonkeyStateCallbacks.lua
-- project: Monkey Stole My GPA
--
-- author:  Chew Shee Yang / How Hong Chin Ryan
--
-- brief:   Lazy Monkey state machine callback implementations in Lua
--
-- Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
-----------------------------------------------------------------------------------

-- ========================================================================
-- Idle State Callbacks
-- ========================================================================

function lazy_monkey_idle_enter(entity)
    local lazy_monkey = entity:get_component("LazyMonkey")
    if lazy_monkey then
        -- Store when we entered idle (for recovery delay after being hit)
        lazy_monkey.last_attack_time = Time.get_time()
    end
    print("Lazy Monkey: Entered Idle state (Lua)")
end

function lazy_monkey_idle_update(entity)
    local state_machine = entity:get_component("StateMachine")
    local lazy_monkey = entity:get_component("LazyMonkey")
    
    if not lazy_monkey then return end
    
    local current_time = Time.get_time()
    local time_in_idle = current_time - lazy_monkey.last_attack_time
    
    -- Don't chase immediately after entering idle (recovery from hit)
    if time_in_idle < 0.3 then
        return
    end
    
    -- Get player entity
    local player_entity = ECS.get_first_entity_with("Player")
    
    if not player_entity:is_valid() then
        print("Error: Player entity not found in Lazy Monkey Idle state update")
        return
    end
    
    -- Check if player is nearby
    if player_entity:is_valid() and lazy_monkey then
        local lazy_monkey_transform = entity:get_component("Transform2D")
        local player_transform = player_entity:get_component("Transform2D")
        
        if lazy_monkey_transform and player_transform then
            local dx = player_transform.position.x - lazy_monkey_transform.position.x
            local dy = player_transform.position.y - lazy_monkey_transform.position.y
            local distance = math.sqrt(dx * dx + dy * dy)
            
            -- Start chasing if player is within detection range
            if distance < lazy_monkey.detection_range then
                state_machine:change_state("Chasing")
            end
        end
    end
end

-- ========================================================================
-- Chasing State Callbacks
-- ========================================================================

function lazy_monkey_chasing_enter(entity)
    print("Lazy Monkey: Entered Chasing state (Lua)")
end

function lazy_monkey_chasing_update(entity)
    local state_machine = entity:get_component("StateMachine")
    local rigid = entity:get_component("RigidBody")
    local lazy_monkey_transform = entity:get_component("Transform2D")
    local flip = entity:get_component("Flip")
    local lazy_monkey = entity:get_component("LazyMonkey")
    
    if not lazy_monkey then return end
    
    local current_time = Time.get_time()
    
    -- Get player entity
    local player_entity = ECS.get_first_entity_with("Player")
    
    if player_entity:is_valid() and rigid and lazy_monkey_transform then
        local player_transform = player_entity:get_component("Transform2D")
        
        if player_transform then
            local dx = player_transform.position.x - lazy_monkey_transform.position.x
            local distance = math.abs(dx)
            
            -- Chase player horizontally
            if distance > 10.0 then
                rigid.velocity.x = dx > 0 and lazy_monkey.chase_speed or -lazy_monkey.chase_speed
                
                -- Update flip direction based on movement
                if flip then
                    flip.value.x = dx > 0 and -1.0 or 1.0
                end
            end
            
            -- Return to idle if player is far away
            if distance > lazy_monkey.chase_range then
                state_machine:change_state("Idle")
            end
            
            -- Attack if close enough AND cooldown has elapsed
            if distance < lazy_monkey.attack_range and
               (current_time - lazy_monkey.last_attack_time) >= lazy_monkey.attack_cooldown then
                lazy_monkey.last_attack_time = current_time
                state_machine:change_state("Attacking")
            end
        end
    end
end

function lazy_monkey_chasing_exit(entity)
    -- Cleanup if needed
end

-- ========================================================================
-- Attacking State Callbacks
-- ========================================================================

function lazy_monkey_attacking_enter(entity)
    local rigid = entity:get_component("RigidBody")
    local flip = entity:get_component("Flip")
    local lazy_monkey = entity:get_component("LazyMonkey")
    
    if rigid and flip and lazy_monkey and rigid.on_ground then
        -- Throw monkey forward based on facing direction
        rigid.velocity.x = flip.value.x > 0 and -lazy_monkey.leap_speed or lazy_monkey.leap_speed
        -- Add upward velocity for a leap effect
        rigid.velocity.y = -lazy_monkey.leap_height
    end
    
    print("Lazy Monkey: Entered Attacking state (Lua)")
end

function lazy_monkey_attacking_update(entity)
    local state_machine = entity:get_component("StateMachine")
    local animation = entity:get_component("Animation")
    
    -- Transition back to chasing when attack animation completes
    if animation and animation:has_ended() then
        state_machine:change_state("Chasing")
    end
end

function lazy_monkey_attacking_exit(entity)
    local lazy_monkey = entity:get_component("LazyMonkey")
    if lazy_monkey then
        -- Update last attack time for cooldown
        lazy_monkey.last_attack_time = Time.get_time()
    end
end

-- ========================================================================
-- Hit State Callbacks
-- ========================================================================

function lazy_monkey_hit_enter(entity)
    local rigid = entity:get_component("RigidBody")
    local flip = entity:get_component("Flip")
    local lazy_monkey = entity:get_component("LazyMonkey")
    
    print("Lazy Monkey: Stunned! (Lua)")
end

function lazy_monkey_hit_update(entity)
    local state_machine = entity:get_component("StateMachine")
    local lazy_monkey = entity:get_component("LazyMonkey")
    local rigid = entity:get_component("RigidBody")
    local animation = entity:get_component("Animation")
    local pending_knockback = entity:get_component("PendingKnockback")
    
    if not lazy_monkey then return end
    
    -- When animation ends, detach from hitbox and apply knockback
    if animation:has_ended() and pending_knockback and rigid then
        -- Detach from hitbox first (stop following player)
        if entity:has_component("Parent") then
            entity:remove_component("Parent")
        end
        
        -- Apply knockback momentum
        rigid.velocity = pending_knockback.knockback
        
        -- Remove the pending knockback component (one-time use)
        entity:remove_component("PendingKnockback")
    end
    
    -- Transition to idle only when grounded (after knockback has been applied)
    if animation:has_ended() and rigid and rigid.on_ground then
        state_machine:change_state("Idle")
    end
end

function lazy_monkey_hit_exit(entity)
    -- Cleanup: ensure parent is removed (safety check)
    if entity:has_component("Parent") then
        entity:remove_component("Parent")
    end
end

-- ========================================================================
-- Death State Callbacks
-- ========================================================================

function lazy_monkey_death_enter(entity)
    print("Lazy Monkey: Died! (Lua)")
end

function lazy_monkey_death_update(entity)
    local animation = entity:get_component("Animation")
    if animation and animation:has_ended() then
        ECS.destroy_entity(entity)
        print("Lazy Monkey: Destroyed after death animation (Lua)")
    end
end

function lazy_monkey_death_exit(entity)
    -- Should not happen unless respawning
end