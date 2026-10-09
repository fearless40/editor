#pragma once
#include <concepts>
#include <type_traits>
namespace geom {
template<typename T>
concept is_unit = T::is_unit_v;

template<typename T>
concept numeric_c = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
concept is_dimension_c = T::is_dimension;

template<typename T>
concept is_strong_type_c = T::is_strong_type_v;

template<typename SelfT, typename OtherT>
concept is_convertable_to_c = requires { OtherT::unit_t::template ConvertTo<SelfT, OtherT>; };

template<typename SelfT, typename OtherT>
concept is_convertable_from_c = requires { SelfT::unit_t::template ConvertFrom<SelfT, OtherT>; };


template<typename T, typename U>
concept dimension_is_same_c = std::is_same_v<T, U> and is_dimension_c<T>;

namespace concepts {
  // 1. Helper to safely check if the flag exists and is true
  template<typename T>
  concept has_disallowed_negation = requires {
    { T::disallow_negation_v } -> std::convertible_to<bool>;
  } && T::disallow_negation_v;
}// namespace concepts

// Define static constexpr bool disallow_negation_v = true to prevent operator -()
template<typename T>
concept allow_negation_c = is_dimension_c<T> and !concepts::has_disallowed_negation<T>;

}// namespace geom
