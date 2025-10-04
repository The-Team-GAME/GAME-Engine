//-----------------------------------------------------------------------------------
// file:    Vector3D.cpp
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
#include "Vector3D.h"

namespace Math {

  // Constructor
  // Vec3D(f32 x_vec, f32 y_vec) : x{ x_vec }, y{ y_vec } {}
  // Acessors
  f32 Vec3D::X() const { return x; }
  f32 Vec3D::Y() const { return y; }
  f32 Vec3D::Z() const { return z; }
  // Mutators
  f32& Vec3D::X() { return x; }
  f32& Vec3D::Y() { return y; }
  f32& Vec3D::Z() { return z; }
  void Vec3D::zero() { x = 0; y = 0; z = 0; }
  // Member Functions
  f32 Vec3D::get_magnitude_sq() const
  {
    return powf(x, 2) + powf(y, 2) + powf(z, 2);
  }

  f32 Vec3D::get_magnitude() const
  {
    return sqrt(this->get_magnitude_sq());
  }

  // Member Function Operator Overloads
  Vec3D& Vec3D::operator+=(Vec3D const& rhs)
  {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }
  Vec3D& Vec3D::operator-=(Vec3D const& rhs)
  {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }
  Vec3D& Vec3D::operator- () {
    x *= -1;
    y *= -1;
    z *= -1;
    return *this;
  }
  Vec3D& Vec3D::operator*=(f32 scalar)
  {
    *this = *this * scalar;
    return *this;
  }

  // Non Member Functions
  Vec3D normalise_vector(Vec3D const& vect)
  {
    f32 magnitude = vect.get_magnitude();
    Vec3D tmp{ vect };
    return tmp / magnitude;
  }

  Vec3D scale_vector(Vec3D const& vect, f32 scale)
  {
    Vec3D tmp{ vect };
    return tmp * scale;
  }

  f32 dot_product(Vec3D const& lhs, Vec3D const& rhs)
  {
    return	lhs.x * rhs.x +
      lhs.y * rhs.y +
      lhs.z * rhs.z;
  }

  Vec3D cross_product(Vec3D const& lhs, Vec3D const& rhs)
  {
    Vec3D tmp
    {
      lhs.y * rhs.z - lhs.z * rhs.y,
      lhs.z * rhs.x - lhs.x * rhs.z,
      lhs.x * rhs.y - lhs.y * rhs.x
    };
    return tmp;
  }

  // Non Member Operator Overloads
  Vec3D operator+(Vec3D const& vect1, Vec3D const& vect2)
  {
    Vec3D tmp{ vect1 };
    tmp += vect2;
    return tmp;
  }
  Vec3D operator-(Vec3D const& vect1, Vec3D const& vect2)
  {
    Vec3D tmp{ vect1 };
    tmp -= vect2;
    return tmp;
  }
  Vec3D operator/(Vec3D const& vect, f32 divisor)
  {
    Vec3D tmp{ vect };
    tmp.x = vect.x / divisor;
    tmp.y = vect.y / divisor;
    tmp.z = vect.z / divisor;
    return tmp;
  }
  Vec3D operator*(Vec3D const& vect, f32 scalar)
  {
    Vec3D tmp{ vect };
    tmp.x = vect.x * scalar;
    tmp.y = vect.y * scalar;
    tmp.z = vect.z * scalar;
    return tmp;
  }

  std::ostream& operator<<(std::ostream& os, Vec3D const& vect)
  {
    os << "(" << vect.x << "," << vect.y << "," << vect.z << ")";
    return os;
  }
}

