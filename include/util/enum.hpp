#pragma once

#include <type_traits>
#include <utility>

namespace util::enum_helper {
template <class T>
concept IsEnum = std::is_enum_v<T>;

template <IsEnum EnumT> constexpr auto rep(EnumT e) {
  return std::to_underlying(e);
}
} // namespace util::enum_helper

namespace util::flags {

template <class EnumT> struct DeclareFlagEnum {
  static constexpr bool is_flag = false;
};

#define MAKE_ENUM_FLAG(x)                                                      \
  template <> struct util::flags::DeclareFlagEnum<x> {                         \
    static constexpr bool is_flag = true;                                      \
  };

#define MAKE_ENUM_FLAG_IN_CLASS(x)                                             \
  template <class EnumT> struct DeclareFlagEnum {                              \
    static constexpr bool is_flag = false;                                     \
  };                                                                           \
  template <> struct DeclareFlagEnum<x> {                                      \
    static constexpr bool is_flag = true;                                      \
  };

template <class EnumT>
concept IsFlagEnum =
    std::is_enum_v<EnumT> && DeclareFlagEnum<EnumT>::is_flag == true;

template <IsFlagEnum EnumT>
constexpr EnumT operator&(const EnumT lhs, const EnumT rhs) {
  return EnumT{static_cast<std::underlying_type_t<EnumT>>(
      std::to_underlying(lhs) & std::to_underlying(rhs))};
};

template <IsFlagEnum EnumT>
constexpr EnumT operator|(const EnumT lhs, const EnumT rhs) {
  return EnumT{static_cast<std::underlying_type_t<EnumT>>(
      std::to_underlying(lhs) | std::to_underlying(rhs))};
};

template <IsFlagEnum EnumT>
constexpr EnumT remove(const EnumT flags, const EnumT flag_to_remove) {
  return EnumT{static_cast<std::underlying_type_t<EnumT>>(
      std::to_underlying(flags) & ~std::to_underlying(flag_to_remove))};
};

template <IsFlagEnum EnumT>
constexpr EnumT operator^(const EnumT lhs, const EnumT rhs) {
  return EnumT{static_cast<std::underlying_type_t<EnumT>>(
      std::to_underlying(lhs) ^ std::to_underlying(rhs))};
}
} // namespace util::flags
