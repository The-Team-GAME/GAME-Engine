#include "DebugDrawSystem.h"
using namespace Engine;
using namespace Systems;
using ddsys = DebugDrawSystem;
using col2D = Collision2DSystem;

void ddsys::draw()
{
	col2D::draw_quad_tree();
}