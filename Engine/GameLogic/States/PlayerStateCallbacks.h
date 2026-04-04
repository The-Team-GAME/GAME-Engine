//-----------------------------------------------------------------------------------
// file:    PlayerStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (90%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   Player state machine callback declarations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../Components/GameplayComponent.h"
namespace PlayerStateCallbacks
{
	// Ensure that the abilities on cooldown will come off cooldown once the timer is over
	void handle_cooldowns(ECS::Entity entity);
	
	// Handleing of the input that the player gives
	void take_input(ECS::Entity entity); 
	// attack and movement input called in take_input
	bool take_attack_input(ECS::Entity entity);
	bool take_movement_input(ECS::Entity entity);
	// Prevents the player from moving through keyboard inputs when we dont want them to
	void player_input_lock();
	void player_input_unlock();

	// After a checkpoint death the scene is reloaded; call this in GameplayState::load()
	// to retrieve the stored checkpoint position and place the player there.
	// Returns true (and clears the pending value) if a checkpoint respawn is pending.
	bool consume_pending_checkpoint_respawn(glm::vec2& out_pos);
}