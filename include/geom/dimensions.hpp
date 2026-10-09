#pragma once
#include "concepts.hpp"
#include "strongtype.hpp"
#include <type_traits>

namespace geom {

template<typename TagT> struct Position;
template<typename TagT> struct Distance;


namespace concepts {
  template<typename StrongT>
  concept strongtype_dimension_is_position_c =
    std::is_same_v<typename StrongT::dimension_t, Position<typename StrongT::dimension_t::tag_t>>;

  template<typename StrongT>
  concept strongtype_dimension_is_distance_c =
    std::is_same_v<typename StrongT::dimension_t, Distance<typename StrongT::dimension_t::tag_t>>;


};// namespace concepts

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

template<typename TAG_T> struct Area
{
  using tag_t = TAG_T;
  static constexpr bool is_dimension = true;
};


template<typename TAG_T> struct Distance : helpers::DimensionPlus
{
  using tag_t = TAG_T;
  static constexpr bool is_dimension = true;


  // Distance * Scalar = Distance
  template<is_strong_type_c T, typename ValueT>
    requires std::is_same_v<typename T::underlying_t, ValueT>
  static constexpr T multi(const T &lhs, ValueT value)
  { return T{ lhs.value * value }; }

  // Distance * Distance  = Area
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
    requires concepts::strongtype_dimension_is_distance_c<LhsT> && concepts::strongtype_dimension_is_distance_c<RhsT>
  static constexpr auto multi(const LhsT &lhs, const RhsT &rhs) noexcept
  {
    return StrongType<typename LhsT::underlying_t, Area<typename LhsT::dimension_t::tag_t>, typename LhsT::unit_t>{
      lhs.value * rhs.value
    };
  }


  // Distance + Distance = Distance
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
    requires concepts::strongtype_dimension_is_distance_c<LhsT> and concepts::strongtype_dimension_is_distance_c<RhsT>
  static constexpr LhsT plus(const LhsT &lhs, const RhsT &rhs)
  { return LhsT{ lhs.value + rhs.value }; }

  // Distance - Distance = Distance
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
    requires concepts::strongtype_dimension_is_distance_c<LhsT> and concepts::strongtype_dimension_is_distance_c<RhsT>
  static constexpr LhsT minus(const LhsT &lhs, const RhsT &rhs)
  { return LhsT{ lhs.value - rhs.value }; }
};

template<typename TAG_T> struct Position : helpers::DimensionMulti
{
  static constexpr bool is_dimension = true;
  using tag_t = TAG_T;

  // Postion - Position = Distance
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
  static constexpr auto minus(const LhsT &lhs, const RhsT &rhs) noexcept
    requires concepts::strongtype_dimension_is_position_c<LhsT> && std::is_constructible_v<LhsT, RhsT>
  { return StrongType<typename LhsT::underlying_t, Distance<tag_t>, typename LhsT::unit_t>{ lhs.value - rhs.value }; }


  // Distance + Position = Position
  // std::is_same_v<typename LhsT::dimension_t, Position<tag_t>>
  //              && std::is_same_v<typename RhsT::dimension_t, Distance<tag_t>>)
  //            || (std::is_same_v<typename LhsT::dimension_t, Distance<tag_t>>
  //                && std::is_same_v<typename RhsT::dimension_t, Position<tag_t>>)
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
    requires(
      (concepts::strongtype_dimension_is_position_c<LhsT> && concepts::strongtype_dimension_is_distance_c<RhsT>)
      || (concepts::strongtype_dimension_is_distance_c<LhsT> && concepts::strongtype_dimension_is_position_c<RhsT>))
  static constexpr auto plus(const LhsT &lhs, const RhsT &rhs)
  {
    return StrongType<typename LhsT::underlying_t, Position<typename LhsT::dimension_t::tag_t>, typename LhsT::unit_t>{
      lhs.value + rhs.value
    };
  }

  // Position - Distance = Position
  // std::is_same_v<typename LhsT::dimension_t, Position<typename LhsT::dimension_t::tag_t>>
  //              && std::is_same_v<typename RhsT::dimension_t, Distance<typename RhsT::dimension_t::tag_t>>
  template<is_strong_type_c LhsT, is_strong_type_c RhsT>
    requires(concepts::strongtype_dimension_is_position_c<LhsT> && concepts::strongtype_dimension_is_distance_c<RhsT>)
  static constexpr auto minus(const LhsT &lhs, const RhsT &rhs)
  {
    return StrongType<typename LhsT::underlying_t, Position<typename LhsT::dimension_t::tag_t>, typename LhsT::unit_t>{
      lhs.value - rhs.value
    };
  }
};


// Converts a Position to a distance by subtracting the origin stored in Units
template<is_strong_type_c LhsT>
  requires concepts::strongtype_dimension_is_position_c<LhsT>
constexpr auto origin(const LhsT &lhs) noexcept
{ return lhs - LhsT{ LhsT::unit_t::template origin<typename LhsT::underlying_t> }; }
}// namespace geom
