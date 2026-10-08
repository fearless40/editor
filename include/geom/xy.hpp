#pragma once

#include "dimensions.hpp"
#include "strongtype.hpp"
#include "units.hpp"

namespace geom {


// clang-format off
struct X_tag { };
struct Y_tag { };
struct Row_tag{};
struct Col_tag{};
// clang-format on

using X = StrongType<int, Position<X_tag>, AbsolutePosition>;
using Y = StrongType<int, Position<Y_tag>, AbsolutePosition>;
using Row = Y;
using Col = X;

using Width = StrongType<int, Distance<X_tag>, AbsolutePosition>;
using Height = StrongType<int, Distance<Y_tag>, AbsolutePosition>;


}// namespace geom
