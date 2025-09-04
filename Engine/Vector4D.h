#pragma once
#include "TypeDef.h"
#include <iostream>

namespace Math {

	namespace Vector4D {

		class Vec4D {
		private:
			f64 x, y, z, w;
		public:
			Vec4D(f64 x_vec = 0, f64 y_vec = 0, f64 z_vec = 0, f64 w_vec = 0) 
				: x{ x_vec }, y{ y_vec }, z{ z_vec }, w{ w_vec } {}; //(default)ctor
			Vec4D(Vec4D const&) = default; //copy constructor

			// Acessors
			f64 X() const;
			f64 Y() const;
			f64 Z() const;
			f64 W() const;
			// Mutators
			void X(f64 xval);
			void Y(f64 yval);
			void Z(f64 zval);
			void W(f64 wval);
			void zero();
			// Member functions
			f64 get_magnitude_sq() const;
			f64 get_magnitude() const;

			// Member function operator overloads
			Vec4D& operator+=(Vec4D const& rhs);
			Vec4D& operator-=(Vec4D const& rhs);
			Vec4D& operator- ();
			Vec4D& operator*=(f64 scalar);
		};

		// Non-member functions
		Vec4D normalise_vector(Vec4D const& vect);
		Vec4D scale_vector(Vec4D const& vect, f64 scale);
		f64 dot_product(Vec4D const& lhs, Vec4D const& rhs);

		// Non-member operator overloads
		Vec4D operator+(Vec4D const& vect1, Vec4D const& vect2);
		Vec4D operator-(Vec4D const& vect1, Vec4D const& vect2);
		Vec4D operator/(Vec4D const& vect, f64 divisor);
		Vec4D operator*(Vec4D const& vect, f64 scalar);
		std::ostream& operator<<(std::ostream& os, Vec4D const& vect);
	}
}
