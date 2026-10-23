#pragma once

// #include <array>
// #include <cstdint>
// // #include <fmt/ranges.h>
//
// namespace cpp23 {
// constexpr std::uint64_t log2(std::uint64_t x) { return __builtin_log2(x); }
// constexpr auto log10(std::uint64_t x) { return long(__builtin_ceil(__builtin_log10(x))); }
// constexpr std::uint64_t pow(std::uint64_t x, std::uint64_t e) { return __builtin_pow(x, e); }
// }// namespace cpp23
//
// inline int digit_count(uint32_t x)
// {
//   static constexpr std::array<std::uint64_t, 32> table = []() {
//     std::array<std::uint64_t, 32> table;
//     for (unsigned long i = 1; i < 33; i++) {
//       const unsigned smallest = cpp23::pow(2, i - 1);
//       table[i - 1] =
//         (i < 31 ? (cpp23::pow(2, 32) - cpp23::pow(10, cpp23::log10(smallest))) : 0) + (cpp23::log10(smallest) << 32);
//     }
//     return table;
//   }();
//   // fmt::print("{}", table);
//   return (x + table[31 - __builtin_clz(x | 1)]) >> 32;
// }
//
//
#include <cstdint>

namespace util {
namespace detail {
  inline int int_log2(uint32_t x) { return 31 - __builtin_clz(x | 1); }
}// namespace detail


inline int fast_digit_count(uint32_t x)
{
  static uint64_t table[] = { 4294967296,
    8589934582,
    8589934582,
    8589934582,
    12884901788,
    12884901788,
    12884901788,
    17179868184,
    17179868184,
    17179868184,
    21474826480,
    21474826480,
    21474826480,
    21474826480,
    25769703776,
    25769703776,
    25769703776,
    30063771072,
    30063771072,
    30063771072,
    34349738368,
    34349738368,
    34349738368,
    34349738368,
    38554705664,
    38554705664,
    38554705664,
    41949672960,
    41949672960,
    41949672960,
    42949672960,
    42949672960 };
  return (x + table[detail::int_log2(x)]) >> 32;
}
}// namespace util
