#pragma once
#include "concepts.hpp"
#include <compare>
namespace geom {
template<typename UnderlyingT, typename DimensionT, typename UnitT> struct StrongType
{
  static constexpr bool is_strong_type_v = true;
  using underlying_t = UnderlyingT;
  using difference_t = underlying_t;
  using dimension_t = DimensionT;
  using unit_t = UnitT;
  using type = StrongType<underlying_t, DimensionT, UnitT>;

  template<typename T>
    requires std::is_same_v<underlying_t, T> && numeric_c<T>
  constexpr StrongType(const T &val) : value(static_cast<underlying_t>(val))
  {}

  template<is_strong_type_c T>
    requires is_convertable_to_c<type, T> || is_convertable_from_c<type, T>
  constexpr StrongType(const T &val)
    : value([&]() {
        if constexpr (is_convertable_to_c<type, T>) {
          return T::unit_t::template ConvertTo<type, T>(val);
        } else if (is_convertable_from_c<type, T>) {
          return unit_t::template ConvertFrom<type, T>(val);
        }
      }())
  {}

  constexpr StrongType(const type &other) = default;

  underlying_t value{ 0 };

  // Get underlying value
  constexpr underlying_t underlying() const noexcept { return value; }


  // Comparison
  constexpr std::strong_ordering operator<=>(type const &other) const noexcept = default;


  constexpr std::strong_ordering operator<=>(underlying_t const &other) const noexcept { return value <=> other; }

  template<typename OtherT> constexpr std::strong_ordering operator<=>(OtherT const &other) const noexcept = delete;


  constexpr type &operator+=(type const &other) noexcept
  {
    value += other.value;
    return *this;
  }
  constexpr type &operator-=(type const &other) noexcept
  {
    value -= other.value;
    return *this;
  }
  constexpr type &operator++() noexcept
  {
    ++value;// += static_cast<underlying_t>(1);
    return *this;
  }
  constexpr type operator++(int) noexcept
  {
    type ret = *this;
    ++value;// += static_cast<underlying_t>(1);
    return ret;
  }
  constexpr type &operator--() noexcept
  {
    --value;// -= static_cast<underlying_t>(1);
    return *this;
  }

  constexpr type &operator--(int) noexcept
  {
    auto ret = *this;
    --value;// -= static_cast<underlying_t>(1);
    return ret;
  }

  constexpr type &operator*=(underlying_t value_) noexcept
  {
    value *= value_;
    return *this;
  }


  friend constexpr type operator-(const type &left, const type &right) noexcept
  {
    if constexpr (std::is_unsigned_v<underlying_t>) {
      if (right.value > left.value) { return type{ 0 }; }
    }

    return type{ left.value - right.value };
  }
};

template<is_strong_type_c LhsT, is_strong_type_c RhsT>
  requires(
    requires(const LhsT &l, const RhsT &r) { LhsT::dimension_t::template plus<LhsT, RhsT>(l, r); }
    || requires(const LhsT &l, const RhsT &r) { RhsT::dimension_t::template plus<LhsT, RhsT>(l, r); })
constexpr auto operator+(const LhsT &left, const RhsT &right) noexcept
{
  if constexpr (requires { LhsT::dimension_t::template plus<LhsT, RhsT>; }) {
    return LhsT::dimension_t::plus(left, right);
  } else {
    return RhsT::dimension_t::plus(left, right);
  }
}


template<is_strong_type_c LhsT, is_strong_type_c RhsT>
  requires(
    requires(const LhsT &l, const RhsT &r) { LhsT::dimension_t::template multi<LhsT, RhsT>(l, r); }
    || requires(const LhsT &l, const RhsT &r) { RhsT::dimension_t::template multi<LhsT, RhsT>(l, r); })
constexpr auto operator*(const LhsT &left, const RhsT &right) noexcept
{
  if constexpr (requires { LhsT::dimension_t::template multi<LhsT, RhsT>; }) {
    return LhsT::dimension_t::multi(left, right);
  } else {
    return RhsT::dimension_t::multi(left, right);
  }
}


// DimenstionT * Sclar Value = DimenstionT
template<is_strong_type_c LhsT, typename RhsT>
  requires(requires(const LhsT &l, const RhsT &r) { LhsT::dimension_t::template multi<LhsT, RhsT>(l, r); }
           && std::is_same_v<typename LhsT::underlying_t, RhsT>)
constexpr auto operator*(const LhsT &left, const RhsT right) noexcept
{ return LhsT::dimension_t::multi(left, right); }

}// namespace geom
