#include "Vector4D.h"

namespace Math {

  // Constructor
  // Vec4D(float x_vec, float y_vec) : x{ x_vec }, y{ y_vec } {}
  // Acessors
  float Vec4D::X() const { return x; }
  float Vec4D::Y() const { return y; }
  float Vec4D::Z() const { return z; }
  float Vec4D::W() const { return w; }
  // Mutators
  void Vec4D::X(float xval) { x = xval; }
  void Vec4D::Y(float yval) { y = yval; }
  void Vec4D::Z(float zval) { z = zval; }
  void Vec4D::W(float wval) { w = wval; }
  void Vec4D::zero() { x = 0; y = 0; z = 0; w = 0; }
  // Member Functions
  float Vec4D::get_magnitude_sq() const
  {
    return pow(x, 2) + pow(y, 2) + pow(z, 2) + pow(w, 2);
  }

  float Vec4D::get_magnitude() const
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
  Vec4D& Vec4D::operator*=(float scalar)
  {
    *this = *this * scalar;
    return *this;
  }

  // Non Member Functions
  Vec4D normalise_vector(Vec4D const& vect)
  {
    float magnitude = vect.get_magnitude();
    Vec4D tmp{ vect };
    return tmp / magnitude;
  }

  Vec4D scale_vector(Vec4D const& vect, float scale)
  {
    Vec4D tmp{ vect };
    return tmp * scale;
  }

  float dot_product(Vec4D const& lhs, Vec4D const& rhs)
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
  Vec4D operator/(Vec4D const& vect, float divisor)
  {
    Vec4D tmp{ vect };
    tmp.X(vect.X() / divisor);
    tmp.Y(vect.Y() / divisor);
    tmp.Z(vect.Z() / divisor);
    tmp.W(vect.W() / divisor);
    return tmp;
  }
  Vec4D operator*(Vec4D const& vect, float scalar)
  {
    Vec4D tmp{ vect };
    tmp.X(vect.X() * scalar);
    tmp.Y(vect.Y() * scalar);
    tmp.Z(vect.Z() * scalar);
    tmp.W(vect.W() * scalar);
    return tmp;
  }

  std::ostream& operator<<(std::ostream& os, Vec4D const& vect)
  {
    os << "(" << vect.X() << "," << vect.Y() << "," << vect.Z() << ")";
    return os;
  }
}

