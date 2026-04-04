//-----------------------------------------------------------------------------------
// file:    Vector4D.h
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
#pragma once

#include "../TypeDef.h"
#include "../EngineExport.h"
#include <iostream>
#include <glm/glm.hpp>
#include "../Reflection/ReflectionLibrary.h"

namespace Math {


  class ENGINE_API Vec4D {
  public:

    f32 x, y, z, w;

    /*=================================================================================
    Function Brief:
    The function clears the rapidjson::Document variables that holds the parsed file
    stream.

    Where to call it:
    Usually after a json file has been read and the values parsed into their appropriate
    variables or before another json file is about to be read. Clearing the
    json Document basically ensures that nothing foreign ends up in the new stream that
    you are going to read.
    ==================================================================================*/
    Vec4D(f32 x_vec = 0, f32 y_vec = 0, f32 z_vec = 0, f32 w_vec = 0)
      : x{ x_vec }, y{ y_vec }, z{ z_vec }, w{ w_vec } {
    }; //(default)ctor
    Vec4D(Vec4D const&) = default; //copy constructor

    // Constructor from glm::vec4
    Vec4D(const glm::vec4& v) : x{ v.x }, y{ v.y }, z{ v.z }, w{ v.w } {}

    /* ======== //
    // Acessors //
    // ======== */
    f32 X() const;
    f32 Y() const;
    f32 Z() const;
    f32 W() const;

    /* ======== //
    // Mutators //
    // ======== */
    f32& X();
    f32& Y();
    f32& Z();
    f32& W();
    // Sets the vector to become a zero vector
    void zero();

    /* ================ //
    // Member functions //
    // ================ */
    f32 get_magnitude_sq() const;
    f32 get_magnitude() const;

    /* ================================== //
    // Member function operator overloads //
    // ================================== */
    Vec4D& operator+=(Vec4D const& rhs);
    Vec4D& operator-=(Vec4D const& rhs);
    Vec4D& operator- ();
    Vec4D& operator*=(f32 scalar);

    /* ========================= //
    // Conversion to GLM types   //
    // ========================= */
    operator glm::vec4() const { return glm::vec4(x, y, z, w); }
  };

  /* ===================== //
  // Non-member functions  //
  // ===================== */
  ENGINE_API Vec4D normalise_vector(Vec4D const& vect);
  ENGINE_API Vec4D scale_vector(Vec4D const& vect, f32 scale);
  ENGINE_API f32 dot_product(Vec4D const& lhs, Vec4D const& rhs);

  /* ============================= //
  // Non-member operator overloads //
  // ============================= */
  // Takes in 2 vectors to compute and returns the result
  ENGINE_API Vec4D operator+(Vec4D const& vect1, Vec4D const& vect2);
  ENGINE_API Vec4D operator-(Vec4D const& vect1, Vec4D const& vect2);
  ENGINE_API Vec4D operator/(Vec4D const& vect, f32 divisor);
  ENGINE_API Vec4D operator*(Vec4D const& vect, f32 scalar);
  // Overload of the << operator to display the vector in the output stream
  ENGINE_API std::ostream& operator<<(std::ostream& os, Vec4D const& vect);
}

REFLECT_FIELDS(Math::Vec4D, x, y, z, w)