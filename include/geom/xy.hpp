#pragma once

#include "relative_dimension.hpp"
#include "typed_position.hpp"
#include "typed_scalar.hpp"

namespace geom {

namespace detail {
  struct AbsoluteUNIT
  {
  };
  struct X_TAG
  {
  };
  struct Y_TAG
  {
  };

  struct Area_TAG
  {
  };
}// namespace detail

using X = TypedPosition<int, detail::X_TAG, detail::AbsoluteUNIT>;
using Y = TypedPosition<int, detail::Y_TAG, detail::AbsoluteUNIT>;


using Width = TypedDimension<X>;
using Height = TypedDimension<Y>;
using Area = TypedScalar<long, detail::Area_TAG>;

constexpr Area operator*(Width w, Height h)
{ return Area{ static_cast<Area::underlying_t>(w.underlying()) * static_cast<Area::underlying_t>(h.underlying()) }; }

}// namespace geom
