#pragma once

#include <compare>
#include <concepts>
#include <type_traits>
namespace geom {

template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

template<Numeric UnderlyingT, typename TAG, typename UnitT> struct TypedPosition
{
  using underlying_t = UnderlyingT;
  using difference_type = underlying_t;
  using type = TypedPosition<underlying_t, TAG, UnitT>;

  template<typename ValueT>
    requires std::convertible_to<underlying_t, ValueT>
  constexpr TypedPosition(ValueT val) : value(static_cast<underlying_t>(val))
  {}


  constexpr TypedPosition(const type &other) : value(other.underlying()) {}

  underlying_t value{ 0 };

  // Get underlying value
  constexpr underlying_t underlying() const noexcept { return value; }


  // Conversion
  constexpr operator underlying_t() const noexcept { return value; }
  template<typename OtherT> operator OtherT() const = delete;// prevent other conversions

  template<typename ValueT>
    requires std::convertible_to<underlying_t, ValueT>
  [[nodiscard]] constexpr ValueT to() const noexcept
  { return static_cast<ValueT>(value); }

  // Comparison
  constexpr std::strong_ordering operator<=>(type const &other) const noexcept = default;


  constexpr std::strong_ordering operator<=>(underlying_t const &other) const noexcept { return value <=> other; }

  template<typename OtherT> constexpr std::strong_ordering operator<=>(OtherT const &other) const noexcept = delete;

  // constexpr bool operator==(type const &other) const noexcept = default;
  // constexpr bool operator!=(type const &other) const noexcept = default;

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

  constexpr type &operator=(underlying_t value_) noexcept
  {
    value = value_;
    return *this;
  }

  constexpr type &operator=(const type &other) noexcept = default;

  friend constexpr type operator+(const type &left, const type &right) noexcept
  { return type{ left.value + right.value }; }


  friend constexpr type operator-(const type &left, const type &right) noexcept
  {
    if constexpr (std::is_unsigned_v<underlying_t>) {
      if (right.value > left.value) { return type{ 0 }; }
    }

    return type{ left.value - right.value };
  }
};

}// namespace geom
