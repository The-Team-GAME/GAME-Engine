---------------------------------------------------------------------------------- -
// file:    Vector2D.cpp
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
#include "Vector2D.h"

namespace Math {

  // Constructor
  // Vec2D(f32 x_vec, f32 y_vec) : x{ x_vec }, y{ y_vec } {}
  // Acessors
  f32 Vec2D::X() const { return x; }
  f32 Vec2D::Y() const { return y; }
  // Mutators
  void Vec2D::X(f32 xval) { x = xval; }
  void Vec2D::Y(f32 yval) { y = yval; }
  void Vec2D::zero() { x = 0; y = 0; }
  // Member Functions
  f32 Vec2D::get_magnitude_sq() const
  {
    return powf(x, 2) + powf(y, 2);
  }

  f32 Vec2D::get_magnitude() const
  {
    return sqrt(this->get_magnitude_sq());
  }

  // Member Function Operator Overloads
  Vec2D& Vec2D::operator+=(Vec2D const& rhs)
  {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
  Vec2D& Vec2D::operator-=(Vec2D const& rhs)
  {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }
  Vec2D& Vec2D::operator- () {
    x *= -1;
    y *= -1;
    return *this;
  }
  Vec2D& Vec2D::operator*=(f32 scalar)
  {
    *this = *this * scalar;
    return *this;
  }

  // Non Member Functions
  Vec2D normalise_vector(Vec2D const& vect)
  {
    f32 magnitude = vect.get_magnitude();
    Vec2D tmp{ vect };
    return tmp / magnitude;
  }

  Vec2D scale_vector(Vec2D const& vect, f32 scale)
  {
    Vec2D tmp{ vect };
    return tmp * scale;
  }

  f32 dot_product(Vec2D const& lhs, Vec2D const& rhs)
  {
    return lhs.X() * rhs.X() + lhs.Y() * rhs.Y();
  }
  f32 cross_product(Vec2D const& lhs, Vec2D const& rhs)
  {
    f32 result = lhs.X() * rhs.Y() - lhs.Y() * rhs.X();
    return result < 0 ? -result : result;
  }

  // Non Member Operator Overloads
  Vec2D operator+(Vec2D const& vect1, Vec2D const& vect2)
  {
    Vec2D tmp{ vect1 };
    tmp += vect2;
    return tmp;
  }
  Vec2D operator-(Vec2D const& vect1, Vec2D const& vect2)
  {
    Vec2D tmp{ vect1 };
    tmp -= vect2;
    return tmp;
  }
  Vec2D operator/(Vec2D const& vect, f32 divisor)
  {
    Vec2D tmp{ vect };
    tmp.X(vect.X() / divisor);
    tmp.Y(vect.Y() / divisor);
    return tmp;
  }
  Vec2D operator*(Vec2D const& vect, f32 scalar)
  {
    Vec2D tmp{ vect };
    tmp.X(vect.X() * scalar);
    tmp.Y(vect.Y() * scalar);
    return tmp;
  }

  std::ostream& operator<<(std::ostream& os, Vec2D const& vect)
  {
    os << "(" << vect.X() << "," << vect.Y() << ")";
    return os;
  }
}

