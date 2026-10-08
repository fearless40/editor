#pragma once
#include "concepts.hpp"
#include "strongtype.hpp"

namespace geom {

namespace helpers {
  struct DimensionPlus
  {
    template<is_strong_type_c T> static constexpr T plus(const T &lhs, const T &rhs)
    { return T{ lhs.value + rhs.value }; }

    template<is_strong_type_c T> static constexpr T minus(const T &lhs, const T &rhs)
    { return T{ lhs.value - rhs.value }; }
  };

  struct DimensionMulti
  {
    template<is_strong_type_c T> static constexpr T multi(const T &lhs, const T &rhs)
    { return T{ lhs.value * rhs.value }; }

    template<is_strong_type_c T> static constexpr T div(const T &lhs, const T &rhs)
    { return T{ lhs.value / rhs.value }; }
  };
}// namespace helpers

template<typename TAG_T> struct Distance : helpers::DimensionPlus
{
  using tag_t = TAG_T;
  static constexpr bool is_dimension = true;


  // Distance * Scalar = Distance
  template<is_strong_type_c T, typename ValueT>
    requires std::is_same_v<typename T::underlying_t, ValueT>
  static constexpr T multi(const T &lhs, ValueT value)
  { return T{ lhs.value * value }; }
};

template<typename TAG_T> struct Position : helpers::DimensionMulti
{
  static constexpr bool is_dimension = true;
  using tag_t = TAG_T;

  // Postion - Position = Distance
  template<is_strong_type_c T> static constexpr auto minus(const T &lhs, const T &rhs)
  { return StrongType<typename T::underlying_t, Distance<tag_t>, typename T::unit_t>{ lhs.value - rhs.value }; }


  // Distance + Position = Position
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
    requires(std::is_same_v<typename LhsT::dimension_t, Position<tag_t>>
              && std::is_same_v<typename RhsT::dimension_t, Distance<tag_t>>)
            || (std::is_same_v<typename LhsT::dimension_t, Distance<tag_t>>
                && std::is_same_v<typename RhsT::dimension_t, Position<tag_t>>)
  static constexpr auto plus(const LhsT &lhs, const RhsT &rhs)
  { return StrongType<typename LhsT::underlying_t, Position, typename LhsT::unit_t>{ lhs.value + rhs.value }; }


  // Position - Distance = Position
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
    requires(
      std::is_same_v<typename LhsT::unit_t, Position<tag_t>> && std::is_same_v<typename RhsT::unit_t, Distance<tag_t>>)
  static constexpr auto minus(const LhsT &lhs, const RhsT &rhs)
  { return StrongType<typename LhsT::underlying_t, Position, typename LhsT::unit_t>{ lhs.value - rhs.value }; }
};
}// namespace geom
