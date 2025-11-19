//-----------------------------------------------------------------------------------
// file:    DebugDrawSystem.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a system containing functions that will allow the 
//					developers to debug the engine using visual aids.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "DebugDrawSystem.h"
using namespace Engine;
using namespace Systems;
using ddsys = DebugDrawSystem;
using col2D = Collision2DSystem;

void ddsys::draw()
{
	// Only draw if debug layer is active
	if (ECS::is_layer_active("debug")) {
		col2D::draw_quad_tree();
	}
}