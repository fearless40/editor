#pragma once
#include <compare>
#include <concepts>
#include <type_traits>

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
  // }())
  // {
  //   if constexpr (is_convertable_to_c<type, T>) {
  //     value = T::unit_t::template ConvertTo<type, T>(val);
  //   } else if (is_convertable_from_c<type, T>) {
  //     value = unit_t::template ConvertFrom<type, T>(val);
  //   }
  // }


  constexpr StrongType(const type &other) = default;

  underlying_t value{ 0 };

  // Get underlying value
  constexpr underlying_t underlying() const noexcept { return value; }


  // Conversion
  // constexpr operator underlying_t() const noexcept { return value; }
  // template<typename OtherT> operator OtherT() const = delete;// prevent other conversions
  //
  // template<typename ValueT>
  //   requires std::convertible_to<underlying_t, ValueT>
  // [[nodiscard]] constexpr ValueT to() const noexcept
  // { return static_cast<ValueT>(value); }
  //
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

  // template<is_strong_type_c T>
  //   requires is_convertable_to_c<type, T>
  // constexpr type &operator=(const T &value_) noexcept
  // {
  //   value = T::unit_t::ConvertTo(value_);
  //   // value = value_;
  //   return *this;
  // }
  //
  // constexpr type &operator=(const type &other) noexcept = default;
  //
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

struct Width
{
  static constexpr bool is_dimension = true;
};

