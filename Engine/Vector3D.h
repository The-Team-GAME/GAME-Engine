#pragma once

#include "TypeDef.h"
#include <iostream>

namespace Math {

	namespace Vector3D {

		class Vec3D {
		private:
			f64 x, y, z;
		public:
			Vec3D(f64 x_vec = 0, f64 y_vec = 0, f64 z_vec = 0) : x{ x_vec }, y{ y_vec }, z{ z_vec } {}; //(default)ctor
			Vec3D(Vec3D const&) = default; //copy constructor

			// Acessors
			f64 X() const;
			f64 Y() const;
			f64 Z() const;
			// Mutators
			void X(f64 xval);
			void Y(f64 yval);
			void Z(f64 zval);
			void zero();
			// Member functions
			f64 get_magnitude_sq() const;
			f64 get_magnitude() const;

			// Member function operator overloads
			Vec3D& operator+=(Vec3D const& rhs);
			Vec3D& operator-=(Vec3D const& rhs);
			Vec3D& operator- ();
			Vec3D& operator*=(f64 scalar);
		};

		// Non-member functions
		Vec3D normalise_vector(Vec3D const& vect);
		Vec3D scale_vector(Vec3D const& vect, f64 scale);
		f64 dot_product(Vec3D const& lhs, Vec3D const& rhs);
		f64 cross_product(Vec3D const& lhs, Vec3D const& rhs);

		// Non-member operator overloads
		Vec3D operator+(Vec3D const& vect1, Vec3D const& vect2);
		Vec3D operator-(Vec3D const& vect1, Vec3D const& vect2);
		Vec3D operator/(Vec3D const& vect, f64 divisor);
		Vec3D operator*(Vec3D const& vect, f64 scalar);
		std::ostream& operator<<(std::ostream& os, Vec3D const& vect);
	}
}




