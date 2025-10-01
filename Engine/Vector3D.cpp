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
  void Vec3D::X(f32 xval) { x = xval; }
  void Vec3D::Y(f32 yval) { y = yval; }
  void Vec3D::Z(f32 zval) { z = zval; }
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
    return	lhs.X() * rhs.X() +
      lhs.Y() * rhs.Y() +
      lhs.Z() * rhs.Z();
  }

  Vec3D cross_product(Vec3D const& lhs, Vec3D const& rhs)
  {
    Vec3D tmp
    {
      lhs.Y() * rhs.Z() - lhs.Z() * rhs.Y(),
      lhs.Z() * rhs.X() - lhs.X() * rhs.Z(),
      lhs.X() * rhs.Y() - lhs.Y() * rhs.X()
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
    tmp.X(vect.X() / divisor);
    tmp.Y(vect.Y() / divisor);
    tmp.Z(vect.Z() / divisor);
    return tmp;
  }
  Vec3D operator*(Vec3D const& vect, f32 scalar)
  {
    Vec3D tmp{ vect };
    tmp.X(vect.X() * scalar);
    tmp.Y(vect.Y() * scalar);
    tmp.Z(vect.Z() * scalar);
    return tmp;
  }

  std::ostream& operator<<(std::ostream& os, Vec3D const& vect)
  {
    os << "(" << vect.X() << "," << vect.Y() << "," << vect.Z() << ")";
    return os;
  }
}

