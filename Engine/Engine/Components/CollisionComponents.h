//-----------------------------------------------------------------------------------
// file:    CollisionComponents.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file contains the definition of the data structures that will be
//          needed for collision related logic / calculations
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../Math/Vector2D.h"
#include "../ECS/Macros.h"
#include <limits>

struct CollisionResult {
  bool collided = false;
  Vec2D mtv;  // Minimum Translation Vector aka penetration vector

  CollisionResult(bool collision, Vec2D penetration)
    : collided(collision), mtv(penetration)
  {
    // Penetration set to 0 in the event of no collision
  }

  // Helper methods
  Vec2D get_normal() const {
    return normalise_vector(mtv);
  }
  f32 get_penetration() const {
    return mtv.get_magnitude();
  }
};

struct AABB {
  Vec2D pos;
  // Adding this afterwards
  f32 half_width;
  f32 half_height;

  AABB(Vec2D pos = Vec2D(0, 0), f32 width = 0.f, f32 height = 0.f) : pos(pos), half_width(width / 2), half_height(height / 2)
  {
    // Default Ctor
    // Empty by design
  }

  AABB(f32 width, f32 height) : pos(0, 0), half_width(width / 2), half_height(height / 2)
  {
    // For the old style of AABB so it wont return any problems b4 M2 submissions
    // Empty by design
  }

  Vec2D min() const
  {
    return pos - Vec2D(half_width, half_height);
  }
  Vec2D max() const
  {
    return pos + Vec2D(half_width, half_height);
  }

  bool intersects(AABB const& rhs) const
  {
    // This is the logic
    // (lhs_min.x < rhs_max.x) &&
    // (lhs_max.x > rhs_min.x) &&
    // (lhs_min.y < rhs_max.y) &&
    // (lhs_max.y > rhs_min.y);

    return  (pos.x - half_width < rhs.pos.x + rhs.half_width)   &&
            (rhs.pos.x - rhs.half_width < pos.x + half_width)   &&
            (pos.y - half_height < rhs.pos.y + rhs.half_height) &&
            (rhs.pos.y - rhs.half_height < pos.y + half_height);
  }
};

struct Polygon
{
  std::vector<Vec2D> vertices; // In local space (relative to position)
  Vec2D position;              // World position
  f32 rotation;                // Rotation in radians

  Polygon(Vec2D world_pos = Vec2D(0,0)) : position(world_pos), rotation() {};

  // Cpy Ctor from an AABB
  Polygon(const AABB& aabb) : position(aabb.pos), rotation(0) {
    // Create box vertices from AABB dimensions
    f32 hw = aabb.half_width;
    f32 hh = aabb.half_height;
    vertices = {
        Vec2D(-hw, -hh),  // Top-left
        Vec2D(-hw,  hh),  // Bottom-left
        Vec2D(hw,   hh),  // Bottom-right
        Vec2D(hw,  -hh)   // Top-right
    };
  }

  // Helper to create a box polygon
  static Polygon create_box(f32 width, f32 height)
  {
    Polygon poly;
    f32 hw = width / 2;
    f32 hh = height / 2;
    poly.vertices = {
        Vec2D(-hw, -hh),  // Top-left
        Vec2D(-hw,  hh),  // Bottom-left
        Vec2D( hw,  hh),  // Bottom-right
        Vec2D( hw, -hh)   // Top-right
    };
    return poly;
  }

  // Get world-space vertices
  std::vector<Vec2D> get_transformed_vertices() const {
    std::vector<Vec2D> transformed;
    transformed.reserve(vertices.size());

    for (Vec2D const& vec : vertices) {
      // We rotate then translate here because the vertices are relative to the position 
      Vec2D rotated = rotate(vec, rotation);
      transformed.push_back(rotated + position);
    }
    return transformed;
  }

  // For this function, it dosent matter if the vertices are in clockwise or anticlockwise order, as long as they are inserted in the intended order
  std::vector<Vec2D> get_axes() const 
  {
    std::vector<Vec2D> transformed = get_transformed_vertices();
    std::vector<Vec2D> axes;
    axes.reserve(vertices.size());

    for (size_t i = 0; i < transformed.size(); ++i) {
      size_t next = (i + 1) % transformed.size(); // We do this so that last vertice can calculate with the first one in the vector
      Vec2D edge = transformed[next] - transformed[i];

      // Perpendicular to edge (normal)
      Vec2D normal = outward_normal(edge);
      normal = normalise_vector(normal);
      axes.push_back(normal);
    }
    return axes;
  }

  // Projection of polygon onto an axis
  struct Projection 
  {
    f32 min;
    f32 max;

    // Checking if 2 projections overlap
    bool overlaps(const Projection& other) const 
    {
      return !(max < other.min || other.max < min);
    }

    // Checking how much 2 projections overlap when they overlap
    f32 get_overlap(const Projection& other) const 
    {
      return std::min(max, other.max) - std::max(min, other.min);
    }
  };

  // Projecting the polygon onto an axis -> only one ah, means need call once per axis check
  Projection project_onto_axis(Vec2D const& axis) const 
  {
    std::vector<Vec2D> transformed = get_transformed_vertices();

    f32 min = dot_product(axis, transformed[0]);
    f32 max = min;

    for (size_t i = 1; i < transformed.size(); ++i) {
      f32 projection = dot_product(axis, transformed[i]);
      min = projection < min ? projection : min;
      max = projection > max ? projection : max;
    }
    
    return { min, max };
  }

  // The actual collision test, aka final boss :>
  CollisionResult intersects(Polygon const& other) const
  {
    Vec2D smallest_axis;
    f32 smallest_overlap = std::numeric_limits<f32>::max(); // placeholder value

    std::vector<Vec2D> my_axes = get_axes();
    for (Vec2D const& axis : my_axes) {
      Projection my_projection    = project_onto_axis(axis);
      Projection other_projection = other.project_onto_axis(axis);

      // If even one axis dosent overlap, the 2 polygons do not collide
      if (!my_projection.overlaps(other_projection)) 
      {
        // No collision
        return CollisionResult(false, Vec2D(0, 0));
      }

      f32 overlap = my_projection.get_overlap(other_projection);
      if (overlap < smallest_overlap) 
      {
        smallest_overlap = overlap;
        smallest_axis = axis;
      }
    }

    // Now we test again for the other object's axes
    std::vector<Vec2D> other_axes = other.get_axes();
    for (Vec2D const& axis : other_axes) {
      Projection my_projection = project_onto_axis(axis);
      Projection other_projection = other.project_onto_axis(axis);

      // The order of checking dosent matter here :>
      if (!my_projection.overlaps(other_projection)) 
      {
        return CollisionResult(false, Vec2D(0, 0));
      }

      f32 overlap = my_projection.get_overlap(other_projection);
      if (overlap < smallest_overlap) 
      {
        smallest_overlap = overlap;
        smallest_axis = axis;
      }
    }

    // You are worthy, collision true XD
    // Ensure MTV points from other to this (correct direction)
    Vec2D other_to_this = position - other.position;
    // This next part basically checks if the MTV is pointing in the correct direction
    // Since dot product will be positive if smallest_axis is pointing in the same general direction (within +- 90 of other_to_this)
    if (dot_product(other_to_this, smallest_axis) < 0)
    {
      smallest_axis = smallest_axis * -1.0f;
    }

    Vec2D mtv = smallest_axis * smallest_overlap;
    return CollisionResult(true, mtv);
  }

};



namespace Components
{
  enum class ColliderType {
    AABB,      // Axis-aligned (no rotation) - fastest
    OBB,       // Oriented bounding box (with rotation)
    POLYGON    // Custom polygon shape
  };

  // https://docs.unity3d.com/6000.2/Documentation/Manual/class-BoxCollider.html
  struct BoxCollider {
    Engine::ECS::LayerSignature include_layers; // Additional layers to collide with
    Engine::ECS::LayerSignature exclude_layers; // Layers to NOT collide with (overrides include)

    // If a object has resolve_collisions set to true,that means that other objects that 
    // collide into the current object needs to be snapped out of the current object
    bool resolve_collisions = false;
    ColliderType type = ColliderType::AABB;

    // AABB data (for fast axis-aligned checks)
    AABB aabb;
    f32 width;
    f32 height;
    f32 radius;

    // Polygon data (for rotated/custom shapes)
    Polygon polygon;

    // AABB constructor (existing - no rotation)
    BoxCollider(f32 width, f32 height, bool is_static_object)
      : width(width), height(height), resolve_collisions(is_static_object), type(ColliderType::AABB)
    {
      aabb.half_width = width / 2;
      aabb.half_height = height / 2;
    }

    // OBB constructor (rotated box)
    BoxCollider(f32 width, f32 height, f32 rotation, bool is_static_object)
      : width(width), height(height), resolve_collisions(is_static_object), type(ColliderType::OBB)
    {
      polygon = Polygon::create_box(width, height);
      polygon.rotation = rotation;
    }

    // Custom polygon constructor
    BoxCollider(const std::vector<Vec2D>& vertices, bool is_static_object)
      : resolve_collisions(is_static_object), type(ColliderType::POLYGON)
    {
      polygon.vertices = vertices;
      polygon.rotation = 0;
    }

    // Default constructor for reflection
    BoxCollider() : width(0.0f), height(0.0f), radius(0.0f), resolve_collisions(false)
    {
      /* empty by design */
    }
  };
}

REFLECT_FIELDS(AABB, pos, half_width, half_height)
REGISTER_COMPONENT(Components::BoxCollider, resolve_collisions, width, height, radius, aabb)