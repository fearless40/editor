#pragma once

#include "editortypes.hpp"
#include <span>
#include <string_view>
#include <vector>
class FixedWidthTextBuffer
{
  using uint = unsigned int;
  std::vector<std::byte> buff;
  Width line_width{ 1 };
  Height line_count{ 1 };

public:
  // constexpr auto line_view()
  // {
  //   return std::views::chunk(buff, line_width.underlying())
  //          | std::views::transform([](auto chunk) { return std::string_view(chunk); });
  // }
  //
  // constexpr auto line_view_index() { return std::views::enumerate(line_view()); }

  void ensure_size(Width new_width, Height nbr_lines)
  {
    auto max_size = new_width * nbr_lines;
    if (max_size.underlying() > buff.capacity()) { buff.reserve(max_size.underlying()); }
    line_width = new_width;
    line_count = nbr_lines;
  }

  std::string_view view_at(Row row)
  {
    auto *ptr = buff.data();
    // Move ptr by number of rows;
    ptr += static_cast<ptrdiff_t>(row.underlying() * line_width.underlying());
    return std::string_view{ reinterpret_cast<char *>(ptr), reinterpret_cast<char *>(ptr) + line_width.underlying() };
  }

  std::span<std::byte> span_at(Row row)
  {
    auto ptrPostion = row.underlying() * line_width.underlying();
    return std::span<std::byte>{ buff.data() + ptrPostion, static_cast<std::size_t>(line_width.underlying()) };
  }
};
