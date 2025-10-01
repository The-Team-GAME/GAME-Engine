//-----------------------------------------------------------------------------------
// file:    Vector4D.cpp
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
#include "Vector4D.h"

namespace Math {

  // Constructor
  // Vec4D(f32 x_vec, f32 y_vec) : x{ x_vec }, y{ y_vec } {}
  // Acessors
  f32 Vec4D::X() const { return x; }
  f32 Vec4D::Y() const { return y; }
  f32 Vec4D::Z() const { return z; }
  f32 Vec4D::W() const { return w; }
  // Mutators
  f32& Vec4D::X() { return x; }
  f32& Vec4D::Y() { return y; }
  f32& Vec4D::Z() { return z; }
  f32& Vec4D::W() { return w; }
  void Vec4D::zero() { x = 0; y = 0; z = 0; w = 0; }
  // Member Functions
  f32 Vec4D::get_magnitude_sq() const
  {
    return powf(x, 2) + powf(y, 2) + powf(z, 2) + powf(w, 2);
  }

  f32 Vec4D::get_magnitude() const
  {
    return sqrt(this->get_magnitude_sq());
  }

  // Member Function Operator Overloads
  Vec4D& Vec4D::operator+=(Vec4D const& rhs)
  {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    w += rhs.w;
    return *this;
  }
  Vec4D& Vec4D::operator-=(Vec4D const& rhs)
  {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    w -= rhs.w;
    return *this;
  }
  Vec4D& Vec4D::operator- () {
    x *= -1;
    y *= -1;
    z *= -1;
    w *= -1;
    return *this;
  }
  Vec4D& Vec4D::operator*=(f32 scalar)
  {
    *this = *this * scalar;
    return *this;
  }

  // Non Member Functions
  Vec4D normalise_vector(Vec4D const& vect)
  {
    f32 magnitude = vect.get_magnitude();
    Vec4D tmp{ vect };
    return tmp / magnitude;
  }

  Vec4D scale_vector(Vec4D const& vect, f32 scale)
  {
    Vec4D tmp{ vect };
    return tmp * scale;
  }

  f32 dot_product(Vec4D const& lhs, Vec4D const& rhs)
  {
    return	lhs.X() * rhs.X() +
      lhs.Y() * rhs.Y() +
      lhs.Z() * rhs.Z() +
      lhs.W() * rhs.W();
  }

  // Non Member Operator Overloads
  Vec4D operator+(Vec4D const& vect1, Vec4D const& vect2)
  {
    Vec4D tmp{ vect1 };
    tmp += vect2;
    return tmp;
  }
  Vec4D operator-(Vec4D const& vect1, Vec4D const& vect2)
  {
    Vec4D tmp{ vect1 };
    tmp -= vect2;
    return tmp;
  }
  Vec4D operator/(Vec4D const& vect, f32 divisor)
  {
    Vec4D tmp{ vect };
    tmp.X() = vect.X() / divisor;
    tmp.Y() = vect.Y() / divisor;
    tmp.Z() = vect.Z() / divisor;
    tmp.W() = vect.W() / divisor;
    return tmp;
  }
  Vec4D operator*(Vec4D const& vect, f32 scalar)
  {
    Vec4D tmp{ vect };
    tmp.X() = vect.X() * scalar;
    tmp.Y() = vect.Y() * scalar;
    tmp.Z() = vect.Z() * scalar;
    tmp.W() = vect.W() * scalar;
    return tmp;
  }

  std::ostream& operator<<(std::ostream& os, Vec4D const& vect)
  {
    os << "(" << vect.X() << "," << vect.Y() << "," << vect.Z() << ")";
    return os;
  }
}

