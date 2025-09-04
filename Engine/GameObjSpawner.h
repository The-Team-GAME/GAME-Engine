#pragma once

#include "ECS.h"
#include <glm/glm.hpp>

struct Position
{
	float x, y;
}; 

struct Velocity
{
	float x, y;
};

struct DrawableRect
{
	float width, height;
	glm::vec4 color;
};

struct Transform 
{ 
	glm::vec2 position; 
	float rotation; 
	glm::vec2 scale; 
};

class GameObjSpawner
{
public:
	GameObjSpawner(EntityComponentSystem::world& ecsRef);

	// Register systems (movement, drawing, etc.)
	void registerSystems();

	// Spawn functions
	EntityComponentSystem::entity_handle spawnGameObject(float x, float y, float width, float height,
		const glm::vec4& color, float vx, float vy);

private:
	EntityComponentSystem::world& ecs;
};


//struct Input { 
//	bool up, down, left, right, shoot; 
//};
//
//struct Tag { std::string name; };
//struct Lifetime { float remainingTime; };
