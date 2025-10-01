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
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "TypeDef.h"
#include <iostream>

namespace Math {


  class Vec3D {
  private:
    float x, y, z;
  public:

    Vec3D(float x_vec = 0, float y_vec = 0, float z_vec = 0) : x{ x_vec }, y{ y_vec }, z{ z_vec } {}; //(default)ctor
    Vec3D(Vec3D const&) = default; //copy constructor

    /* ======== //
    // Acessors //
    // ======== */
    float X() const;
    float Y() const;
    float Z() const;

    /* ======== //
    // Mutators //
    // ======== */
    void X(float xval);
    void Y(float yval);
    void Z(float zval);
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
    Vec3D& operator+=(Vec3D const& rhs);
    Vec3D& operator-=(Vec3D const& rhs);
    Vec3D& operator- ();
    Vec3D& operator*=(float scalar);
  };

  /* ===================== //
  // Non-member functions  //
  // ===================== */
  Vec3D normalise_vector(Vec3D const& vect);
  Vec3D scale_vector(Vec3D const& vect, float scale);
  float dot_product(Vec3D const& lhs, Vec3D const& rhs);
  Vec3D cross_product(Vec3D const& lhs, Vec3D const& rhs);

  /* ============================= //
  // Non-member operator overloads //
  // ============================= */
  // Takes in 2 vectors to compute and returns the result
  Vec3D operator+(Vec3D const& vect1, Vec3D const& vect2);
  Vec3D operator-(Vec3D const& vect1, Vec3D const& vect2);
  Vec3D operator/(Vec3D const& vect, float divisor);
  Vec3D operator*(Vec3D const& vect, float scalar);
  // Overload of the << operator to display the vector in the output stream
  std::ostream& operator<<(std::ostream& os, Vec3D const& vect);
}




