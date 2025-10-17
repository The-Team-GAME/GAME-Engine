//-----------------------------------------------------------------------------------
// file:    Vector3D.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a 3D vector class and all of the operations that can
//          act on a 3D vector.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../TypeDef.h"
#include <iostream>
#include <glm/glm.hpp>
#include "../Reflection/ReflectionLibrary.h"

namespace Math {


  class Vec3D {
  public:

    f32 x, y, z;

    Vec3D(f32 x_vec = 0, f32 y_vec = 0, f32 z_vec = 0) : x{ x_vec }, y{ y_vec }, z{ z_vec } {}; //(default)ctor
    Vec3D(Vec3D const&) = default; //copy constructor
    
    // Constructor from glm::vec3
    Vec3D(const glm::vec3& v) : x{ v.x }, y{ v.y }, z{ v.z } {}

    /* ======== //
    // Acessors //
    // ======== */
    f32 X() const;
    f32 Y() const;
    f32 Z() const;

    /* ======== //
    // Mutators //
    // ======== */
    f32& X();
    f32& Y();
    f32& Z();
    // Sets the vector to become a zero vector
    void zero();

    /* ================ //
    // Member functions //
    // ================ */
    f32 get_magnitude_sq() const;
    f32 get_magnitude() const;

    /* ================================== //
    // Member function operator overloads //
    // ================================== */
    Vec3D& operator+=(Vec3D const& rhs);
    Vec3D& operator-=(Vec3D const& rhs);
    Vec3D& operator- ();
    Vec3D& operator*=(f32 scalar);
    
    /* ========================= //
    // Conversion to GLM types   //
    // ========================= */
    operator glm::vec3() const { return glm::vec3(x, y, z); }
  };

  /* ===================== //
  // Non-member functions  //
  // ===================== */
  Vec3D normalise_vector(Vec3D const& vect);
  Vec3D scale_vector(Vec3D const& vect, f32 scale);
  f32 dot_product(Vec3D const& lhs, Vec3D const& rhs);
  Vec3D cross_product(Vec3D const& lhs, Vec3D const& rhs);

  /* ============================= //
  // Non-member operator overloads //
  // ============================= */
  // Takes in 2 vectors to compute and returns the result
  Vec3D operator+(Vec3D const& vect1, Vec3D const& vect2);
  Vec3D operator-(Vec3D const& vect1, Vec3D const& vect2);
  Vec3D operator/(Vec3D const& vect, f32 divisor);
  Vec3D operator*(Vec3D const& vect, f32 scalar);
  // Overload of the << operator to display the vector in the output stream
  std::ostream& operator<<(std::ostream& os, Vec3D const& vect);
}

REFLECT_FIELDS(Math::Vec3D, x, y, z)