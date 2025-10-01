//-----------------------------------------------------------------------------------
// file:    Vector4D.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a 4D vector class and all of the operations that can
//          act on a 4D vector.
//          I implemented this file as i thought that it might be needed for graphics
//          calculations down the line.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "TypeDef.h"
#include <iostream>

namespace Math {


  class Vec4D {
  public:
    float x, y, z, w;

    Vec4D(float x_vec = 0, float y_vec = 0, float z_vec = 0, float w_vec = 0)
      : x{ x_vec }, y{ y_vec }, z{ z_vec }, w{ w_vec } {
    }; //(default)ctor
    Vec4D(Vec4D const&) = default; //copy constructor

    /* ======== //
    // Acessors //
    // ======== */
    float X() const;
    float Y() const;
    float Z() const;
    float W() const;

    /* ======== //
    // Mutators //
    // ======== */
    void X(float xval);
    void Y(float yval);
    void Z(float zval);
    void W(float wval);
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
    Vec4D& operator+=(Vec4D const& rhs);
    Vec4D& operator-=(Vec4D const& rhs);
    Vec4D& operator- ();
    Vec4D& operator*=(float scalar);
  };

  /* ===================== //
  // Non-member functions  //
  // ===================== */
  Vec4D normalise_vector(Vec4D const& vect);
  Vec4D scale_vector(Vec4D const& vect, float scale);
  float dot_product(Vec4D const& lhs, Vec4D const& rhs);

  /* ============================= //
  // Non-member operator overloads //
  // ============================= */
  // Takes in 2 vectors to compute and returns the result
  Vec4D operator+(Vec4D const& vect1, Vec4D const& vect2);
  Vec4D operator-(Vec4D const& vect1, Vec4D const& vect2);
  Vec4D operator/(Vec4D const& vect, float divisor);
  Vec4D operator*(Vec4D const& vect, float scalar);
  // Overload of the << operator to display the vector in the output stream
  std::ostream& operator<<(std::ostream& os, Vec4D const& vect);
}
