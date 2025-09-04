#include "Vector4D.h"

namespace Math {

	namespace Vector4D {
		// Constructor
		// Vec4D(f64 x_vec, f64 y_vec) : x{ x_vec }, y{ y_vec } {}
		// Acessors
		f64 Vec4D::X() const { return x; }
		f64 Vec4D::Y() const { return y; }
		f64 Vec4D::Z() const { return z; }
		f64 Vec4D::W() const { return w; }
		// Mutators
		void Vec4D::X(f64 xval) { x = xval; }
		void Vec4D::Y(f64 yval) { y = yval; }
		void Vec4D::Z(f64 zval) { z = zval; }
		void Vec4D::W(f64 wval) { w = wval; }
		void Vec4D::zero() { x = 0; y = 0; z = 0; w = 0; }
		// Member Functions
		f64 Vec4D::get_magnitude_sq() const
		{
			return pow(x, 2) + pow(y, 2) + pow(z, 2) + pow(w,2);
		}

		f64 Vec4D::get_magnitude() const
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
		Vec4D& Vec4D::operator*=(f64 scalar)
		{
			*this = *this * scalar;
			return *this;
		}

		// Non Member Functions
		Vec4D normalise_vector(Vec4D const& vect)
		{
			f64 magnitude = vect.get_magnitude();
			Vec4D tmp{ vect };
			return tmp / magnitude;
		}

		Vec4D scale_vector(Vec4D const& vect, f64 scale)
		{
			Vec4D tmp{ vect };
			return tmp * scale;
		}

		f64 dot_product(Vec4D const& lhs, Vec4D const& rhs)
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
		Vec4D operator/(Vec4D const& vect, f64 divisor)
		{
			Vec4D tmp{ vect };
			tmp.X(vect.X() / divisor);
			tmp.Y(vect.Y() / divisor);
			tmp.Z(vect.Z() / divisor);
			tmp.W(vect.W() / divisor);
			return tmp;
		}
		Vec4D operator*(Vec4D const& vect, f64 scalar)
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

}
