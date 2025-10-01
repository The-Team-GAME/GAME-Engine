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
    float x, y;

  public:

    Vec2D(float x_vec = 0, float y_vec = 0) : x{ x_vec }, y{ y_vec } {}; //(default)ctor
    Vec2D(Vec2D const&) = default; //copy constructor

    /* ======== //
    // Acessors //
    // ======== */
    float X() const;
    float Y() const;

    /* ======== //
    // Mutators //
    // ======== */
    void X(float xval);
    void Y(float yval);
    // Sets the vector to become a zero vector
    void zero();

    /* ================ //
    // Member functions //
    // ================ */
    float get_magnitude_sq() const;
    float get_magnitude() const;

    /* ================================== //
    // Member function operator overloads //
    // ================================== */
    Vec2D& operator+=(Vec2D const& rhs);
    Vec2D& operator-=(Vec2D const& rhs);
    Vec2D& operator- ();
    Vec2D& operator*=(float scalar);
  };

  /* ===================== //
  // Non-member functions  //
  // ===================== */
  Vec2D normalise_vector(Vec2D const& vect);
  Vec2D scale_vector(Vec2D const& vect, float scale);
  float dot_product(Vec2D const& lhs, Vec2D const& rhs);
  // This one not really cross product, more like finding area of //gram formed
  float cross_product(Vec2D const& lhs, Vec2D const& rhs);


  /* ============================= //
  // Non-member operator overloads //
  // ============================= */
  // Takes in 2 vectors to compute and returns the result
  Vec2D operator+(Vec2D const& vect1, Vec2D const& vect2);
  Vec2D operator-(Vec2D const& vect1, Vec2D const& vect2);
  Vec2D operator/(Vec2D const& vect, float divisor);
  Vec2D operator*(Vec2D const& vect, float scalar);
  // Overload of the << operator to display the vector in the output stream
  std::ostream& operator<<(std::ostream& os, Vec2D const& vect);
}

