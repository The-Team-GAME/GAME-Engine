//-----------------------------------------------------------------------------------
// file:    Vector2D.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a 2D vector class and all of the operations that can
//          act on a 2D vector.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "TypeDef.h"

#include <iostream>

namespace Math {


  class Vec2D {
  private:
    f32 x, y;

  public:

    Vec2D(f32 x_vec = 0, f32 y_vec = 0) : x{ x_vec }, y{ y_vec } {}; //(default)ctor
    Vec2D(Vec2D const&) = default; //copy constructor

    /* ======== //
    // Acessors //
    // ======== */
    f32 X() const;
    f32 Y() const;

    /* ======== //
    // Mutators //
    // ======== */
    void X(f32 xval);
    void Y(f32 yval);
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
    Vec2D& operator+=(Vec2D const& rhs);
    Vec2D& operator-=(Vec2D const& rhs);
    Vec2D& operator- ();
    Vec2D& operator*=(f32 scalar);
  };

  /* ===================== //
  // Non-member functions  //
  // ===================== */
  Vec2D normalise_vector(Vec2D const& vect);
  Vec2D scale_vector(Vec2D const& vect, f32 scale);
  f32 dot_product(Vec2D const& lhs, Vec2D const& rhs);
  // This one not really cross product, more like finding area of //gram formed
  f32 cross_product(Vec2D const& lhs, Vec2D const& rhs);


  /* ============================= //
  // Non-member operator overloads //
  // ============================= */
  // Takes in 2 vectors to compute and returns the result
  Vec2D operator+(Vec2D const& vect1, Vec2D const& vect2);
  Vec2D operator-(Vec2D const& vect1, Vec2D const& vect2);
  Vec2D operator/(Vec2D const& vect, f32 divisor);
  Vec2D operator*(Vec2D const& vect, f32 scalar);
  // Overload of the << operator to display the vector in the output stream
  std::ostream& operator<<(std::ostream& os, Vec2D const& vect);
}

