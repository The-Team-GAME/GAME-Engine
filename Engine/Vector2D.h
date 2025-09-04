#pragma once

#include "TypeDef.h"

#include <iostream>

namespace Math {

	namespace Vector2D {

		class Vec2D {
		private:
			f64 x, y;
		public:
			Vec2D(f64 x_vec = 0, f64 y_vec = 0) : x{ x_vec }, y{ y_vec } {}; //(default)ctor
			Vec2D(Vec2D const&) = default; //copy constructor

			// Acessors
			f64 X() const;
			f64 Y() const;
			// Mutators
			void X(f64 xval);
			void Y(f64 yval);
			void zero();
			// Member functions
			f64 get_magnitude_sq() const;
			f64 get_magnitude() const;

			// Member function operator overloads
			Vec2D& operator+=(Vec2D const& rhs);
			Vec2D& operator-=(Vec2D const& rhs);
			Vec2D& operator- ();
			Vec2D& operator*=(f64 scalar);
		};

		// Non-member functions
		Vec2D normalise_vector(Vec2D const& vect);
		Vec2D scale_vector(Vec2D const& vect, f64 scale);
		f64 dot_product(Vec2D const& lhs, Vec2D const& rhs);
		// This one not really cross product, more like finding area of //gram formed
		f64 cross_product(Vec2D const& lhs, Vec2D const& rhs); 

		// Non-member operator overloads
		Vec2D operator+(Vec2D const& vect1, Vec2D const& vect2);
		Vec2D operator-(Vec2D const& vect1, Vec2D const& vect2);
		Vec2D operator/(Vec2D const& vect, f64 divisor);
		Vec2D operator*(Vec2D const& vect, f64 scalar);
		std::ostream& operator<<(std::ostream& os, Vec2D const& vect);
	}
}

