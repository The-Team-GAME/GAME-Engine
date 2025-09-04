#include "GameObjSpawner.h"
#include "Graphics.h"
#include <glm/glm.hpp>

GameObjSpawner::GameObjSpawner(EntityComponentSystem::world& ecsRef) : ecs(ecsRef) {}

EntityComponentSystem::entity_handle GameObjSpawner::spawnGameObject(float x, float y, float width, float height,
    const glm::vec4& color, float vx, float vy)
{
    return ecs.entity()
        .insert([=](Position& p, Velocity& v, DrawableRect& r) {
        p = { x, y };
        v = { vx, vy };
        r = { width, height, color };
            });
}



void GameObjSpawner::registerSystems()
{
    ecs.system<Position, const Velocity>()
        .each([](Position& p, const Velocity& v) {
        p.x += v.x;
        p.y += v.y;
            });

    ecs.system<const Position, const DrawableRect>()
        .each([](const Position& p, const DrawableRect& r) {
        Graphics::drawRect(p.x, p.y, r.width, r.height, r.color);
            });
}
