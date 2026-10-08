#pragma once

#include <type_traits>
namespace geom {
template<typename T>
concept is_unit = T::is_unit_v;

template<typename T>
concept numeric_c = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
concept is_dimension_c = T::is_dimension_v;

template<typename T>
concept is_strong_type_c = T::is_strong_type_v;

template<typename SelfT, typename OtherT>
concept is_convertable_to_c = requires { OtherT::unit_t::template ConvertTo<SelfT, OtherT>; };

template<typename SelfT, typename OtherT>
concept is_convertable_from_c = requires { SelfT::unit_t::template ConvertFrom<SelfT, OtherT>; };


template<typename T, typename U>
concept dimension_is_same_c = std::is_same_v<T, U> and is_dimension_c<T>;

}// namespace geom
