#pragma once
#include "concepts.hpp"

namespace geom {
struct AbsolutePosition
{
  static constexpr bool is_unit_v = true;
  template<typename T> static constexpr T origin = 0;
};


// Rather than starting at 0 starts at 1
struct AbsolutePositionBase1
{
  static constexpr bool is_unit_v = true;

  template<typename T> static constexpr T origin = 1;

  // ConvertTo and ConvertFrom return the underlying value and expects the calling function to convert to the strong
  // type

  template<is_strong_type_c OtherT, is_strong_type_c SelfT> static constexpr auto ConvertTo(const SelfT &val)
  { return val.underlying() - origin<typename OtherT::underlying_t>; }


  template<is_strong_type_c SelfT, is_strong_type_c OtherT> static constexpr auto ConvertFrom(const OtherT &val)
  { return val.underlying() + origin<typename OtherT::underlying_t>; }
};


}// namespace geom
