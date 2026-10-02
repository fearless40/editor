#pragma once

#include <compare>
#include <concepts>
#include <memory>
#include <type_traits>
namespace geom {

namespace detail {
  struct DefaultConverter
  {
  };
}// namespace detail

template<typename T, typename Tag, typename Converter = detail::DefaultConverter> struct TypedScalar
{

  using type = TypedScalar<T, Tag, Converter>;
  using underlying_t = T;

  underlying_t value;

  constexpr std::strong_ordering operator<=>(type const &other) const noexcept = default;

  constexpr underlying_t underlying() const noexcept { return value; }

  constexpr operator underlying_t() const { return value; }

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
    value += static_cast<underlying_t>(1);
    return *this;
  }
  constexpr type operator++(int) noexcept
  {
    auto ret = value;
    value += static_cast<underlying_t>(1);
    return type{ ret };
  }
  constexpr type &operator--() noexcept
  {
    value -= static_cast<underlying_t>(1);
    return *this;
  }

  constexpr type operator--(int) noexcept
  {
    auto ret = *this;
    value -= static_cast<underlying_t>(1);
    return type{ ret };
  }

  constexpr type &operator*=(underlying_t value_) noexcept
  {
    value *= value_;
    return *this;
  }
  constexpr type &operator/=(underlying_t value_) noexcept
  {
    value /= value_;
    return *this;
  }

  template<class ValueT>
    requires std::convertible_to<underlying_t, ValueT>
  constexpr ValueT to() const noexcept
  { return static_cast<ValueT>(value); }

  // template<typename ValueT>
  //   requires std::convertible_to<ValueT, underlying_t>
  // constexpr type &operator=(ValueT val) noexcept
  // {
  //   value = static_cast<underlying_t>(val);
  //   return *this;
  // }
  //
  template<typename ValueT>
    requires std::convertible_to<ValueT, underlying_t>
  constexpr static type make(ValueT val)
  { return type{ static_cast<underlying_t>(val) }; }

  friend constexpr type operator+(const type &left, const type &right) noexcept
  { return type{ left.value + right.value }; }

  template<typename RightT>
    requires std::convertible_to<RightT, underlying_t>
  friend constexpr type operator+(const type &left, RightT right)
  { return type{ left.underlying() + static_cast<underlying_t>(right) }; }

  friend constexpr type operator-(const type &left, const type &right) noexcept
  { return type{ left.value - right.value }; }
};

}// namespace geom
