#pragma once
#include "concepts.hpp"

namespace geom {
struct AbsolutePosition
{
  static constexpr bool is_unit_v = true;
};

struct AbsolutePositionBase1
{
  static constexpr bool is_unit_v = true;

  template<is_strong_type_c OtherT, is_strong_type_c SelfT> static constexpr auto ConvertTo(const SelfT &val)
  {
    return /*StrongType<typename OtherT::underlying_t, typename OtherT::dimension_t, typename OtherT::unit_t>{*/
      val.underlying() - 1;
    // };
  }


  template<is_strong_type_c SelfT, is_strong_type_c OtherT> static constexpr auto ConvertFrom(const OtherT &val)
  {
    return /*StrongType<typename SelfT::underlying_t, typename SelfT::dimension_t, AbsolutePositionBase1>{*/
      val.underlying() + 1;
    // };
  }
};


}// namespace geom
