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