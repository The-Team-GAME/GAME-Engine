#pragma once
#include "Core.h"


namespace EditorUI {
	class ComponentSelector
	{
	public:
		static void DrawAddComponentButtons(ECS::EntityID entityId);
		static void DrawComponentList(ECS::EntityID entityId);
	};
}

