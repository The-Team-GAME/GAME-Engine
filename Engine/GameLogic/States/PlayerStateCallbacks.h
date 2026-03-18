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
namespace PlayerStateCallbacks
{
	// Prevents the player from moving through keyboard inputs when we dont want them to
	void player_input_lock();
	void player_input_unlock();
}