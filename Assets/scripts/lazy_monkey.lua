-- lazy_monkey.lua
-- Lazy Monkey state machine callbacks (Lua port of LazyMonkeyStateCallbacks.cpp)

-- ========================================================================
-- Helper: Update knockback direction based on player position
-- ========================================================================
local function update_contact_knockback_direction(entity)
  local attack = entity.Attack
  local lazy_monkey = entity.LazyMonkey
  local enemy_transform = entity.Transform2D

  if not attack or not lazy_monkey or not enemy_transform then return end

  local player_entity = ECS.get_first_entity_with("Player")
  if not player_entity or not player_entity:is_valid() then return end

  local player_transform = player_entity.Transform2D
  if not player_transform then return end

  -- Calculate direction from enemy to player
  local dir_x = player_transform.position.x - enemy_transform.position.x
  local sign = (dir_x >= 0.0) and 1.0 or -1.0

  -- Set knockback with correct direction
  attack.knockback.x = sign * math.abs(lazy_monkey.knockback.x)
  attack.knockback.y = -math.abs(lazy_monkey.knockback.y) -- Negative for upward
end

-- ========================================================================
-- Idle State
-- ========================================================================

function lazy_monkey_idle_on_enter(entity)
  local lazy_monkey = entity.LazyMonkey
  if lazy_monkey then
    -- Store when we entered idle (for recovery delay after being hit)
    lazy_monkey.last_attack_time = Time.get_time()
  end
  Log.info("Lazy Monkey: Entered Idle state")
end

function lazy_monkey_idle_on_update(entity)
  -- Update knockback direction for contact damage
  update_contact_knockback_direction(entity)

  local state_machine = entity.StateMachine
  local lazy_monkey = entity.LazyMonkey

  if not lazy_monkey then return end

  local current_time = Time.get_time()
  local time_in_idle = current_time - lazy_monkey.last_attack_time

  -- Don't chase immediately after entering idle (recovery from hit)
  if time_in_idle < 0.3 then
    return
  end

  -- Get player entity
  local player_entity = ECS.get_first_entity_with("Player")
  if not player_entity or not player_entity:is_valid() then
    return
  end

  local lazy_monkey_transform = entity.Transform2D
  local player_transform = player_entity.Transform2D

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

-- ========================================================================
-- Chasing State
-- ========================================================================

function lazy_monkey_chasing_on_enter(entity)
  Log.info("Lazy Monkey: Entered Chasing state")
end

function lazy_monkey_chasing_on_update(entity)
  -- Update knockback direction for contact damage
  update_contact_knockback_direction(entity)

  local state_machine = entity.StateMachine
  local rigid = entity.RigidBody
  local lazy_monkey_transform = entity.Transform2D
  local flip = entity.Flip
  local lazy_monkey = entity.LazyMonkey

  if not lazy_monkey then return end

  local current_time = Time.get_time()

  -- Get player entity
  local player_entity = ECS.get_first_entity_with("Player")

  if player_entity and player_entity:is_valid() and rigid and lazy_monkey_transform then
    local player_transform = player_entity.Transform2D

    if player_transform then
      local dx = player_transform.position.x - lazy_monkey_transform.position.x
      local distance = math.abs(dx) -- Only horizontal distance for sidescrolling

      -- Chase player horizontally, but stop when within attack range
      if distance > lazy_monkey.attack_range then
        local desired_vx = (dx > 0) and lazy_monkey.chase_speed or -lazy_monkey.chase_speed
        local dt = Time.get_fixed_dt()
        local delta_v = desired_vx - rigid.velocity.x
        local force_x = (delta_v / dt) * rigid.mass -- F = m * dv/dt
        Physics.add_force(entity, force_x, 0.0)
        -- Update flip direction based on movement
        if flip then
          flip.value.x = (dx > 0) and 1.0 or -1.0
        end
      else
        -- Stop moving when close enough to attack
        rigid.velocity.x = 0.0
      end

      -- Return to idle if player is far away
      if distance > lazy_monkey.chase_range then
        state_machine:change_state("Idle")
      end

      -- If close enough and cooldown has elapsed, go to windup before attacking
      if distance < lazy_monkey.attack_range and
         (current_time - lazy_monkey.last_attack_time) >= lazy_monkey.attack_cooldown then
        state_machine:change_state("Windup")
      end
    end
  end
end

function lazy_monkey_chasing_on_exit(entity)
  -- empty
end

-- ========================================================================
-- Windup State
-- ========================================================================

function lazy_monkey_windup_on_enter(entity)
  local lazy_monkey = entity.LazyMonkey
  local rigid = entity.RigidBody

  -- Stop moving during windup
  if rigid then
    rigid.velocity.x = 0.0
  end

  -- Record when windup started (reuse last_attack_time)
  if lazy_monkey then
    lazy_monkey.last_attack_time = Time.get_time()
  end

  Log.info("Lazy Monkey: Winding up for attack")
end

function lazy_monkey_windup_on_update(entity)
  -- Update knockback direction for contact damage
  update_contact_knockback_direction(entity)

  local state_machine = entity.StateMachine
  local lazy_monkey = entity.LazyMonkey
  local flip = entity.Flip

  if not lazy_monkey then return end

  local current_time = Time.get_time()
  local windup_elapsed = current_time - lazy_monkey.last_attack_time

  -- Get player entity to face them during windup
  local player_entity = ECS.get_first_entity_with("Player")

  if player_entity and player_entity:is_valid() then
    local lazy_monkey_transform = entity.Transform2D
    local player_transform = player_entity.Transform2D

    if lazy_monkey_transform and player_transform and flip then
      local dx = player_transform.position.x - lazy_monkey_transform.position.x
      -- Update flip direction to face player during wind-up
      flip.value.x = (dx > 0) and 1.0 or -1.0
    end
  end

  -- After windup duration, attack
  if windup_elapsed >= lazy_monkey.attack_windup then
    lazy_monkey.last_attack_time = current_time
    state_machine:change_state("Attacking")
  end
end

-- ========================================================================
-- Attacking State
-- ========================================================================

function lazy_monkey_attacking_on_enter(entity)
  local rigid = entity.RigidBody
  local flip = entity.Flip
  local lazy_monkey = entity.LazyMonkey

  if rigid and flip and lazy_monkey and rigid.on_ground then
    -- Throw monkey forward based on facing direction
    rigid.velocity.x = (flip.value.x > 0) and lazy_monkey.leap_speed or -lazy_monkey.leap_speed
  end

  -- Ensure the monkey is treated as an active damaging hitbox while attacking
  if lazy_monkey then
    if not entity:has("Attack") then
      -- Add default Attack component, then set damage
      entity:add("Attack")
      local attack = entity.Attack
      if attack then
        attack.damage = lazy_monkey.damage
      end
    end

    -- Make sure knockback vector points correctly
    update_contact_knockback_direction(entity)

    -- Add to damage_player layer so collisions with the player trigger damage callbacks
    entity:add_to_layer("damage_player")
  end

  Log.info("Lazy Monkey: Entered Attacking state")
end

function lazy_monkey_attacking_on_update(entity)
  -- Update knockback direction for contact damage
  update_contact_knockback_direction(entity)

  local state_machine = entity.StateMachine
  local animation = entity.Animation

  -- Prevent the lazy monkey from passing through the player while leaping
  local rigid = entity.RigidBody
  local monkey_collider = entity.BoxCollider
  local monkey_transform = entity.Transform2D

  if rigid and monkey_collider and monkey_transform then
    local player_entity = ECS.get_first_entity_with("Player")
    if player_entity and player_entity:is_valid() then
      local player_collider = player_entity.BoxCollider
      if player_collider then
        -- Use simple AABB overlap to detect contact with the player
        if monkey_collider.aabb:intersects(player_collider.aabb) then
          -- Compute how far to move the monkey horizontally so its collider
          -- just touches the player's collider instead of overlapping
          local dx = player_collider.aabb.pos.x - monkey_collider.aabb.pos.x
          if dx ~= 0.0 then
            local sign = (dx > 0.0) and 1.0 or -1.0
            local target_center_x = player_collider.aabb.pos.x -
              sign * (player_collider.aabb.half_width + monkey_collider.aabb.half_width)
            local delta_x = target_center_x - monkey_collider.aabb.pos.x

            monkey_transform.position.x = monkey_transform.position.x + delta_x
            monkey_collider.aabb.pos.x = monkey_collider.aabb.pos.x + delta_x
            -- Stop horizontal motion once we've collided with the player
            rigid.velocity.x = 0.0
          end
        end
      end
    end
  end

  -- Transition back to chasing when attack animation completes
  if animation and animation:has_ended() then
    state_machine:change_state("Chasing")
  end
end

function lazy_monkey_attacking_on_exit(entity)
  local lazy_monkey = entity.LazyMonkey
  if lazy_monkey then
    -- Update last attack time for cooldown
    lazy_monkey.last_attack_time = Time.get_time()
  end

  -- Stop treating the monkey as an active damage hitbox
  entity:remove_from_layer("damage_player")
end

-- ========================================================================
-- Hit State
-- ========================================================================

function lazy_monkey_hit_on_enter(entity)
  Log.info("Lazy Monkey: Stunned!")
end

function lazy_monkey_hit_on_update(entity)
  -- Update knockback direction for contact damage
  update_contact_knockback_direction(entity)

  local state_machine = entity.StateMachine
  local lazy_monkey = entity.LazyMonkey
  local rigid = entity.RigidBody
  local animation = entity.Animation
  local pending_knockback = entity.PendingKnockback

  if not lazy_monkey then return end

  -- When animation ends, detach from hitbox and apply knockback
  if animation:has_ended() and pending_knockback and rigid then
    -- Detach from hitbox first (stop following player)
    if entity:has("Parent") then
      entity:remove("Parent")
    end

    -- Apply knockback using physics system
    Physics.add_force(entity, pending_knockback.knockback.x, pending_knockback.knockback.y)

    -- Remove the pending knockback component (one-time use)
    entity:remove("PendingKnockback")
  end

  -- Transition to idle only when grounded (after knockback has been applied)
  if animation:has_ended() and rigid and rigid.on_ground then
    state_machine:change_state("Idle")
  end
end

function lazy_monkey_hit_on_exit(entity)
  -- Cleanup: ensure parent is removed (safety check)
  if entity:has("Parent") then
    entity:remove("Parent")
  end
end

-- ========================================================================
-- Death State
-- ========================================================================

function lazy_monkey_death_on_enter(entity)
  Log.info("Lazy Monkey: Died!")
end

function lazy_monkey_death_on_update(entity)
  local animation = entity.Animation
  if animation and animation:has_ended() then
    ECS.destroy_entity(entity)
    Log.info("Lazy Monkey: Destroyed after death animation.")
  end
end

function lazy_monkey_death_on_exit(entity)
  -- Should not happen unless respawning
end

-- ========================================================================
-- Blueprint Registration
-- ========================================================================

register_blueprint("LazyMonkey", {
  initial_state = "Idle",
  states = {
    { name = "Idle",      on_enter = "lazy_monkey_idle_on_enter",      on_update = "lazy_monkey_idle_on_update",      on_exit = "" },
    { name = "Chasing",   on_enter = "lazy_monkey_chasing_on_enter",   on_update = "lazy_monkey_chasing_on_update",   on_exit = "lazy_monkey_chasing_on_exit" },
    { name = "Windup",    on_enter = "lazy_monkey_windup_on_enter",    on_update = "lazy_monkey_windup_on_update",    on_exit = "" },
    { name = "Attacking", on_enter = "lazy_monkey_attacking_on_enter", on_update = "lazy_monkey_attacking_on_update", on_exit = "lazy_monkey_attacking_on_exit" },
    { name = "Hit",       on_enter = "lazy_monkey_hit_on_enter",       on_update = "lazy_monkey_hit_on_update",       on_exit = "lazy_monkey_hit_on_exit" },
    { name = "Death",     on_enter = "lazy_monkey_death_on_enter",     on_update = "lazy_monkey_death_on_update",     on_exit = "lazy_monkey_death_on_exit" },
  }
})
