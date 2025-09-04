#include "Vector2D.h"

namespace Math {

	namespace Vector {
		// Constructor
		// Vec2D(f64 x_vec, f64 y_vec) : x{ x_vec }, y{ y_vec } {}
		// Acessors
		f64 Vec2D::X() const { return x; }
		f64 Vec2D::Y() const { return y; }
		// Mutators
		void Vec2D::X(f64 xval) { x = xval; }
		void Vec2D::Y(f64 yval) { y = yval; }
		void Vec2D::zero() { x = 0; y = 0; }
		// Member Functions
		f64 Vec2D::get_magnitude_sq() const
		{
			return pow(x, 2) + pow(y, 2);
		}

		f64 Vec2D::get_magnitude() const
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
		Vec2D& Vec2D::operator*=(f64 scalar)
		{
			*this = *this * scalar;
			return *this;
		}

		// Non Member Functions
		Vec2D normalise_vector(Vec2D const& vect)
		{
			f64 magnitude = vect.get_magnitude();
			Vec2D tmp{ vect };
			return tmp / magnitude;
		}

		Vec2D scale_vector(Vec2D const& vect, f64 scale)
		{
			Vec2D tmp{ vect };
			return tmp * scale;
		}

		f64 dot_product(Vec2D const& lhs, Vec2D const& rhs)
		{
			return lhs.X() * rhs.X() + lhs.Y() * rhs.Y();
		}
		f64 cross_product(Vec2D const& lhs, Vec2D const& rhs)
		{
			f64 result = lhs.X() * rhs.Y() - lhs.Y() * rhs.X();
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
		Vec2D operator/(Vec2D const& vect, f64 divisor)
		{
			Vec2D tmp{ vect };
			tmp.X(vect.X() / divisor);
			tmp.Y(vect.Y() / divisor);
			return tmp;
		}
		Vec2D operator*(Vec2D const& vect, f64 scalar)
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

}
